// chat-server.js
import express from "express";
import fetch from "node-fetch";
import cors from "cors";

const app = express();
app.use(cors());
app.use(express.json());

app.post("/chat", async (req, res) => {
  const prompt = req.body.prompt || "";
  const r = await fetch("http://localhost:11434/api/generate", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({
      model: "phi",
      prompt: "\Always give short, factual, one-sentence answers. Avoid long explanations.\nUser: " + prompt,
      stream: false
    }),
  });
  const data = await r.json();
  res.json({ reply: data.response });
});

app.listen(3000, () => console.log("Chatbot server running on http://localhost:3000"));
