import express from "express";
import cors from "cors";
import { createServer } from "http";
import { WebSocket, WebSocketServer } from "ws";
import mysql, { RowDataPacket } from "mysql2/promise";
import dotenv from "dotenv";

dotenv.config();

const app = express();
const server = createServer(app);
const wss = new WebSocketServer({ server });

// Middleware
app.use(cors());
app.use(express.json());

interface Measurement extends RowDataPacket {
  id: number;
  temperature: number;
  humidity: number;
  timestamp: Date;
}

// MySQL connection pool
const pool = mysql.createPool({
  host: process.env.DB_HOST || "localhost",
  user: process.env.DB_USER || "root",
  password: process.env.DB_PASSWORD || "",
  database: process.env.DB_NAME || "weather_station",
  waitForConnections: true,
  connectionLimit: 10,
  queueLimit: 0,
});

// WebSocket connection handling
wss.on("connection", (ws: WebSocket) => {
  console.log("New WebSocket client connected");

  ws.on("close", () => {
    console.log("Client disconnected");
  });
});

// API Routes
app.get("/api/weather/current", async (req, res) => {
  try {
    const [rows] = await pool.execute<Measurement[]>(
      "SELECT * FROM measurements ORDER BY timestamp DESC LIMIT 1"
    );
    res.json(rows[0] || null);
  } catch (error) {
    res.status(500).json({ error: "Database error" });
  }
});

app.get("/api/weather/history", async (req, res) => {
  try {
    const [rows] = await pool.execute<Measurement[]>(
      "SELECT * FROM measurements WHERE timestamp >= DATE_SUB(NOW(), INTERVAL 24 HOUR) ORDER BY timestamp ASC"
    );
    res.json(rows);
  } catch (error) {
    res.status(500).json({ error: "Database error" });
  }
});

// ESP32 endpoint to receive sensor data
app.post("/api/weather/update", async (req, res) => {
  const { temperature, humidity } = req.body;

  try {
    await pool.execute(
      "INSERT INTO measurements (temperature, humidity) VALUES (?, ?)",
      [temperature, humidity]
    );

    // Broadcast to all connected WebSocket clients
    wss.clients.forEach((client) => {
      if (client.readyState === WebSocket.OPEN) {
        client.send(JSON.stringify({ temperature, humidity }));
      }
    });

    res.json({ success: true });
  } catch (error) {
    res.status(500).json({ error: "Database error" });
  }
});

const PORT = process.env.PORT || 3001;
server.listen(PORT, () => {
  console.log(`Server running on port ${PORT}`);
});
