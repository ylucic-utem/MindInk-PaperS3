import "jsr:@supabase/functions-js/edge-runtime.d.ts";
import { createClient } from "jsr:@supabase/supabase-js@2";

const supabaseUrl = Deno.env.get("SUPABASE_URL") || "";
const supabaseKey = Deno.env.get("SUPABASE_SERVICE_ROLE_KEY") || "";
const elevenLabsKey = Deno.env.get("ELEVEN_LABS_API_KEY") || "";
const geminiKey = Deno.env.get("GEMINI_API_KEY") || "";

const supabase = createClient(supabaseUrl, supabaseKey);

// Background processing function (not awaited)
const processAudioAsync = async (audioId: string) => {
  try {
    console.log("[process-audio] Starting async processing for:", audioId);

    // Fetch audio record
    const { data: audioData, error: audioError } = await supabase
      .from("audio_records")
      .select("*")
      .eq("id", audioId)
      .single();

    if (audioError || !audioData) {
      console.error("[process-audio] Audio not found:", audioError);
      return;
    }

    // Update status to processing
    await supabase
      .from("audio_records")
      .update({ status: "processing" })
      .eq("id", audioId);

    console.log("[process-audio] Status updated to processing");
    console.log("[process-audio] API Keys present:", {
      elevenLabs: !!elevenLabsKey,
      gemini: !!geminiKey
    });

    if (!elevenLabsKey || !geminiKey) {
      console.error("[process-audio] Missing API keys. Set ELEVEN_LABS_API_KEY and GEMINI_API_KEY as environment variables.");
      await supabase
        .from("audio_records")
        .update({ status: "error" })
        .eq("id", audioId);
      return;
    }

    // Get signed URL for audio file
    const { data: signedUrlData } = await supabase.storage
      .from("audio-files")
      .createSignedUrl(audioData.storage_path, 3600);

    if (!signedUrlData?.signedUrl) {
      throw new Error("Failed to get signed URL");
    }

    // Download audio
    console.log("[process-audio] Downloading audio from:", signedUrlData.signedUrl);
    const audioResponse = await fetch(signedUrlData.signedUrl);

    if (!audioResponse.ok) {
      throw new Error(`Failed to download audio: ${audioResponse.status} ${audioResponse.statusText}`);
    }

    const audioBuffer = await audioResponse.arrayBuffer();
    console.log(`[process-audio] Downloaded ${audioBuffer.byteLength} bytes`);

    // Determine actual MIME type based on file extension
    const isWebM = audioData.file_name.toLowerCase().endsWith('.webm');
    const contentType = isWebM ? 'audio/webm' : 'audio/mpeg';

    // Create FormData for multipart upload
    const formData = new FormData();
    formData.append(
      "file",
      new Blob([audioBuffer], { type: contentType }),
      audioData.file_name
    );
    formData.append("model_id", "scribe_v1");  // Use scribe_v1 for speech-to-text

    // POST audio to ElevenLabs to get transcription
    console.log(`[process-audio] Sending ${contentType} to ElevenLabs...`);
    console.log(`[process-audio] File size: ${audioBuffer.byteLength} bytes, name: ${audioData.file_name}`);

    const elResponse = await fetch("https://api.elevenlabs.io/v1/speech-to-text", {
      method: "POST",
      headers: {
        "xi-api-key": elevenLabsKey,
      },
      body: formData,
    });

    console.log(`[process-audio] ElevenLabs response status: ${elResponse.status}`);

    if (!elResponse.ok) {
      const errorText = await elResponse.text();
      console.error(`[process-audio] ElevenLabs error response:`, errorText);
      throw new Error(`ElevenLabs failed: ${elResponse.status} - ${errorText}`);
    }

    const elData = await elResponse.json();
    const transcription = elData.text;

    if (!transcription) {
      throw new Error("No transcription returned from ElevenLabs");
    }

    console.log("[process-audio] Got transcription (length:", transcription.length, "chars)");

    // Save transcription to bucket
    const transcriptionFileName = `transcription-${audioId}.txt`;
    await supabase.storage
      .from("summaries-text")
      .upload(transcriptionFileName, new TextEncoder().encode(transcription), {
        contentType: "text/plain",
        upsert: true,
      });

    // Save transcription to table
    const { data: transcriptionRow } = await supabase
      .from("transcriptions")
      .insert([{
        audio_id: audioId,
        transcription_text: transcription,
        transcription_storage_path: transcriptionFileName,
      }])
      .select()
      .single();

    console.log("[process-audio] Transcription saved, sending to Gemini...");

    // Send to Gemini
    const geminiResponse = await fetch(
      `https://generativelanguage.googleapis.com/v1beta/models/gemini-2.5-flash:generateContent?key=${geminiKey}`,
      {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({
          contents: [{
            parts: [{
              text: `Proporciona un resumen breve en español del siguiente texto:\n\n${transcription}`,
            }],
          }],
        }),
      }
    );

    if (!geminiResponse.ok) {
      const errorText = await geminiResponse.text();
      console.error(`[process-audio] Gemini error response:`, errorText);
      throw new Error(`Gemini failed: ${geminiResponse.status} - ${errorText}`);
    }

    const geminiData = await geminiResponse.json();
    const summary = geminiData.candidates?.[0]?.content?.parts?.[0]?.text || "";

    if (!summary) {
      throw new Error("No summary generated");
    }

    console.log("[process-audio] Got summary (length:", summary.length, "chars)");

    // Save summary to bucket
    const summaryFileName = `summary-${audioId}.txt`;
    await supabase.storage
      .from("summaries-text")
      .upload(summaryFileName, new TextEncoder().encode(summary), {
        contentType: "text/plain",
        upsert: true,
      });

    // Insert into summaries
    const { data: summaryRow, error: summaryError } = await supabase
      .from("summaries")
      .insert([{
        audio_id: audioId,
        transcription_text: transcription,
        summary_storage_path: summaryFileName,
        status: "done",
      }])
      .select()
      .single();

    if (summaryError) {
      console.error("[process-audio] Failed to insert summary:", summaryError);
      throw new Error(`Summary insert failed: ${summaryError.message}`);
    }

    // Update audio record
    await supabase
      .from("audio_records")
      .update({ summary_id: summaryRow?.id, status: "done" })
      .eq("id", audioId);

    // Trigger infographic generation in background
    if (summaryRow?.id) {
      console.log("[process-audio] Triggering infographic generation for summary:", summaryRow.id);
      const infographicFunctionUrl = `${supabaseUrl}/functions/v1/generate-infographic`;
      
      try {
        const infographicResponse = await fetch(infographicFunctionUrl, {
          method: "POST",
          headers: {
            "Content-Type": "application/json",
            "Authorization": `Bearer ${supabaseKey}`,
          },
          body: JSON.stringify({ summary_id: summaryRow.id }),
        });
        
        if (infographicResponse.ok) {
          console.log("[process-audio] Infographic generation triggered successfully");
        } else {
          console.warn("[process-audio] Infographic generation request returned:", infographicResponse.status);
        }
      } catch (infographicError) {
        console.warn("[process-audio] Failed to trigger infographic generation:", infographicError);
      }
    }

    console.log("[process-audio] Complete!");
  } catch (error) {
    console.error("[process-audio] Error:", error);
    try {
      await supabase
        .from("audio_records")
        .update({ status: "error" })
        .eq("id", audioId);
    } catch (e) {
      console.error("Failed to update error status:", e);
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
    const { audio_id } = await req.json();

    if (!audio_id) {
      return new Response(JSON.stringify({ error: "audio_id required" }), {
        status: 400,
        headers: { "Content-Type": "application/json" },
      });
    }

    // Trigger async processing (don't wait)
    processAudioAsync(audio_id).catch((e) => console.error("Background task failed:", e));

    // Return immediately
    return new Response(JSON.stringify({
      success: true,
      audio_id,
      message: "Summary processing triggered. Check Summaries list later.",
    }), {
      status: 202,
      headers: { "Content-Type": "application/json" },
    });
  } catch (error) {
    console.error("[process-audio] Request error:", error);
    return new Response(JSON.stringify({
      error: error instanceof Error ? error.message : String(error),
    }), {
      status: 500,
      headers: { "Content-Type": "application/json" },
    });
  }
});
