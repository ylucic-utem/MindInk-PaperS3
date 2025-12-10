import "jsr:@supabase/functions-js/edge-runtime.d.ts";
import { createClient } from "jsr:@supabase/supabase-js@2";

const supabaseUrl = Deno.env.get("SUPABASE_URL") || "";
const supabaseKey = Deno.env.get("SUPABASE_SERVICE_ROLE_KEY") || "";
const geminiKey = Deno.env.get("GEMINI_API_KEY") || "";

const supabase = createClient(supabaseUrl, supabaseKey);

function createBulletPointPrompt(summaryText: string): string {
  return `You are an expert content summarizer. Convert this summary into a concise list of key bullet points suitable for an infographic. 

Requirements:
- Create 4-6 key bullet points
- Each point should be brief (5-10 words max)
- Make them visually interesting and relevant
- Use clear, direct language
- Avoid technical jargon

Summary:
${summaryText}

Respond with ONLY the bullet points, one per line, starting with a bullet symbol (•). Do not include any other text or explanations.`;
}

// Generate infographic image from bullet points using gemini-3-pro-image-preview
async function generateInfographicImage(bulletPoints: string): Promise<Uint8Array | null> {
  try {
    console.log("[generate-infographic] Creating infographic via gemini-3-pro-image-preview");

    const imagePrompt = `Create a clean, professional black and white infographic for an e-ink display. 
Include these key points:

${bulletPoints}

Design requirements:
- Use ONLY black (#000000) and white (#FFFFFF) colors (no grayscale)
- Include a clear title at the top
- Use bullet points laid out vertically
- Add simple icons or symbols to illustrate concepts (line drawings only)
- Ensure high contrast and clarity for e-ink display
- Use clean sans-serif fonts
- Leave adequate white space around content
- Optimize for e-ink display (540x960px)`;

    // Use gemini-3-pro-image-preview with streaming API
    const response = await fetch(
      `https://generativelanguage.googleapis.com/v1beta/models/gemini-3-pro-image-preview:streamGenerateContent?key=${geminiKey}`,
      {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({
          contents: [
            {
              parts: [
                {
                  text: imagePrompt,
                },
              ],
            },
          ],
          generationConfig: {
            responseModalities: ["IMAGE", "TEXT"],
            imageConfig: {
              aspectRatio: "3:4",
              imageSize: "1K",
            },
          },
        }),
      }
    );

    if (!response.ok) {
      const errorText = await response.text();
      console.error("[generate-infographic] Gemini API error status:", response.status);
      console.error("[generate-infographic] Gemini API error text:", errorText.substring(0, 500));
      throw new Error(`Gemini API failed: ${response.status}`);
    }

    // Parse response to extract image
    const text = await response.text();
    console.log("[generate-infographic] Raw response length:", text.length);
    console.log("[generate-infographic] Response preview:", text.substring(0, 300));
    
    let imageBuffer: Uint8Array | null = null;
    
    // Try parsing as single JSON array first (streamGenerateContent returns array)
    try {
      console.log("[generate-infographic] Attempting JSON array parse");
      const jsonArray = JSON.parse(text);
      
      if (Array.isArray(jsonArray) && jsonArray.length > 0) {
        console.log(`[generate-infographic] Parsed as array with ${jsonArray.length} item(s)`);
        
        for (let i = 0; i < jsonArray.length; i++) {
          const item = jsonArray[i];
          console.log(`[generate-infographic] Array item ${i} keys:`, Object.keys(item));
          
          if (item.candidates) {
            const candidates = item.candidates;
            console.log(`[generate-infographic] Found ${candidates.length} candidates`);
            
            for (let c = 0; c < candidates.length; c++) {
              const parts = candidates[c]?.content?.parts;
              if (parts) {
                console.log(`[generate-infographic] Candidate ${c} has ${parts.length} part(s)`);
                for (const part of parts) {
                  console.log(`[generate-infographic] Part keys:`, Object.keys(part));
                  
                  if (part.inlineData?.data && part.inlineData?.mimeType) {
                    const mimeType = part.inlineData.mimeType;
                    const data = part.inlineData.data;
                    console.log(`[generate-infographic] Found inlineData - MIME: ${mimeType}, data length: ${data.length}`);
                    
                    if (mimeType.includes("image")) {
                      const binaryString = atob(data);
                      imageBuffer = new Uint8Array(binaryString.length);
                      for (let j = 0; j < binaryString.length; j++) {
                        imageBuffer[j] = binaryString.charCodeAt(j);
                      }
                      console.log(`[generate-infographic] Image extracted: ${imageBuffer.length} bytes`);
                      return imageBuffer;
                    }
                  }
                }
              }
            }
          }
        }
      }
    } catch (e) {
      console.log("[generate-infographic] JSON array parse failed:", String(e));
    }

    console.error("[generate-infographic] No image data found in response");
    return null;
  } catch (error) {
    console.error("[generate-infographic] Image generation error:", error);
    throw error;
  }
}

// Main processing
const generateInfographicAsync = async (summaryId: string) => {
  try {
    console.log("[generate-infographic] Starting async processing for summary:", summaryId);

    // Fetch summary record
    const { data: summaryData, error: summaryError } = await supabase
      .from("summaries")
      .select("*")
      .eq("id", summaryId)
      .single();

    if (summaryError || !summaryData) {
      throw new Error(`Summary not found: ${summaryError?.message}`);
    }

    console.log("[generate-infographic] Summary fetched, text length:", summaryData.summary_text?.length);

    let summaryText = summaryData.summary_text;

    // If text is in storage, download it
    if (!summaryText && summaryData.summary_storage_path) {
      try {
        const { data: textData } = await supabase.storage
          .from("summaries-text")
          .download(summaryData.summary_storage_path);

        if (textData) {
          summaryText = new TextDecoder().decode(await textData.arrayBuffer());
        }
      } catch (e) {
        console.error("[generate-infographic] Failed to download summary text:", e);
      }
    }

    if (!summaryText || summaryText.length === 0) {
      console.error("[generate-infographic] No summary text available");
      return;
    }

    // Create bullet point version
    console.log("[generate-infographic] Converting to bullet points");
    const bulletPointsResponse = await fetch(
      `https://generativelanguage.googleapis.com/v1beta/models/gemini-2.5-flash-lite:generateContent?key=${geminiKey}`,
      {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({
          contents: [
            {
              parts: [
                {
                  text: createBulletPointPrompt(summaryText),
                },
              ],
            },
          ],
        }),
      }
    );

    if (!bulletPointsResponse.ok) {
      throw new Error(`Failed to create bullet points: ${bulletPointsResponse.status}`);
    }

    const bulletPointsData = await bulletPointsResponse.json();
    const bulletPoints =
      bulletPointsData.candidates?.[0]?.content?.parts?.[0]?.text || "";

    if (!bulletPoints) {
      throw new Error("No bullet points generated");
    }

    console.log("[generate-infographic] Bullet points created, generating image");

    // Generate infographic image
    const imageBuffer = await generateInfographicImage(bulletPoints);

    if (!imageBuffer) {
      throw new Error("No image buffer generated");
    }

    // Upload image to storage
    const infographicFileName = `infographic-${summaryId}.png`;
    console.log("[generate-infographic] Uploading image to storage:", infographicFileName);

    const { error: uploadError } = await supabase.storage
      .from("gallery-images")
      .upload(infographicFileName, imageBuffer, {
        contentType: "image/png",
        upsert: true,
      });

    if (uploadError) {
      throw new Error(`Upload failed: ${uploadError.message}`);
    }

    // Create infographics table entry
    console.log("[generate-infographic] Creating infographics table entry");

    const { data: infographicRow, error: insertError } = await supabase
      .from("infographics")
      .insert([
        {
          summary_id: summaryId,
          image_file_name: infographicFileName,
          storage_path: infographicFileName,
          generation_date: new Date().toISOString(),
        },
      ])
      .select()
      .single();

    if (insertError) {
      throw new Error(`Infographic insert failed: ${insertError.message}`);
    }

    // Update summaries table with infographic_id
    console.log("[generate-infographic] Linking infographic to summary");

    await supabase
      .from("summaries")
      .update({ infographic_id: infographicRow?.id })
      .eq("id", summaryId);

    console.log("[generate-infographic] Complete! Infographic ID:", infographicRow?.id);
  } catch (error) {
    console.error("[generate-infographic] Error:", error);
    try {
      await supabase
        .from("summaries")
        .update({ status: "infographic_error", error_message: String(error) })
        .eq("id", summaryId);
    } catch (e) {
      console.error("[generate-infographic] Failed to update error status:", e);
    }
  }
};

Deno.serve(async (req: Request) => {
  if (req.method !== "POST") {
    return new Response(JSON.stringify({ error: "Method not allowed" }), {
      status: 405,
      headers: { "Content-Type": "application/json" },
    });
  }

  try {
    const { summary_id } = await req.json();

    if (!summary_id) {
      return new Response(JSON.stringify({ error: "summary_id required" }), {
        status: 400,
        headers: { "Content-Type": "application/json" },
      });
    }

    if (!geminiKey) {
      return new Response(
        JSON.stringify({
          error: "GEMINI_API_KEY not configured",
        }),
        {
          status: 500,
          headers: { "Content-Type": "application/json" },
        }
      );
    }

    // Start async processing
    generateInfographicAsync(summary_id).catch((e) =>
      console.error("[generate-infographic] Background task failed:", e)
    );

    return new Response(
      JSON.stringify({
        success: true,
        summary_id,
        message: "Infographic generation started. Check back later for results.",
      }),
      {
        status: 202,
        headers: { "Content-Type": "application/json" },
      }
    );
  } catch (error) {
    console.error("[generate-infographic] Request error:", error);
    return new Response(
      JSON.stringify({
        error: error instanceof Error ? error.message : String(error),
      }),
      {
        status: 500,
        headers: { "Content-Type": "application/json" },
      }
    );
  }
});
