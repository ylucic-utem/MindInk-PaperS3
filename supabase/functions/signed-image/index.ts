import "jsr:@supabase/functions-js/edge-runtime.d.ts";
import { createClient } from "jsr:@supabase/supabase-js@2";

const supabaseUrl = Deno.env.get("SUPABASE_URL") || "";
const supabaseKey = Deno.env.get("SUPABASE_SERVICE_ROLE_KEY") || "";

const supabase = createClient(supabaseUrl, supabaseKey);

Deno.serve(async (req: Request) => {
  try {
    const url = new URL(req.url);
    const imageId = url.searchParams.get("id");

    if (!imageId) {
      return new Response(JSON.stringify({ error: "id parameter required" }), {
        status: 400,
        headers: { "Content-Type": "application/json" },
      });
    }

    // Fetch image record to get storage path
    const { data: image, error: imageError } = await supabase
      .from("infographics")
      .select("storage_path")
      .eq("id", imageId)
      .single();

    if (imageError || !image) {
      return new Response(JSON.stringify({ error: "Image not found" }), {
        status: 404,
        headers: { "Content-Type": "application/json" },
      });
    }

    // Generate signed URL for the image file
    const { data: signedUrlData, error: urlError } = await supabase.storage
      .from("infographics")
      .createSignedUrl(image.storage_path, 3600); // 1 hour expiry

    if (urlError || !signedUrlData) {
      return new Response(JSON.stringify({ error: "Failed to generate signed URL" }), {
        status: 500,
        headers: { "Content-Type": "application/json" },
      });
    }

    return new Response(JSON.stringify({ url: signedUrlData.signedUrl }), {
      status: 200,
      headers: { "Content-Type": "application/json" },
    });
  } catch (error) {
    console.error("[signed-image] Error:", error);
    return new Response(JSON.stringify({
      error: error instanceof Error ? error.message : String(error),
    }), {
      status: 500,
      headers: { "Content-Type": "application/json" },
    });
  }
});
