import "jsr:@supabase/functions-js/edge-runtime.d.ts";
import { createClient } from "jsr:@supabase/supabase-js@2";

const supabaseUrl = Deno.env.get("SUPABASE_URL") || "";
const supabaseKey = Deno.env.get("SUPABASE_SERVICE_ROLE_KEY") || "";

const supabase = createClient(supabaseUrl, supabaseKey);

Deno.serve(async (req: Request) => {
  try {
    const url = new URL(req.url);
    const summaryId = url.searchParams.get("id");

    if (!summaryId) {
      return new Response(JSON.stringify({ error: "id parameter required" }), {
        status: 400,
        headers: { "Content-Type": "application/json" },
      });
    }

    // Fetch summary record to get storage path
    const { data: summary, error: summaryError } = await supabase
      .from("summaries")
      .select("summary_storage_path")
      .eq("id", summaryId)
      .single();

    if (summaryError || !summary) {
      return new Response(JSON.stringify({ error: "Summary not found" }), {
        status: 404,
        headers: { "Content-Type": "application/json" },
      });
    }

    // Generate signed URL for the summary file
    const { data: signedUrlData, error: urlError } = await supabase.storage
      .from("summaries-text")
      .createSignedUrl(summary.summary_storage_path, 3600); // 1 hour expiry

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
    console.error("[signed-summary] Error:", error);
    return new Response(JSON.stringify({
      error: error instanceof Error ? error.message : String(error),
    }), {
      status: 500,
      headers: { "Content-Type": "application/json" },
    });
  }
});
