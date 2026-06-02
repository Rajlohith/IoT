let historyData = [];

const historyLimit = 300;

function formatValue(value, suffix) {
  if (value === null || value === undefined || Number.isNaN(value)) return `-- ${suffix}`;
  return `${Number(value).toFixed(2)} ${suffix}`;
}

function formatTime(ts) {
  if (!ts) return "--";
  const d = new Date(ts);
  if (isNaN(d.getTime())) return ts;
  return d.toLocaleString();
}

function resizeCanvas(canvas) {
  const rect = canvas.getBoundingClientRect();
  const dpr = window.devicePixelRatio || 1;
  canvas.width = Math.round(rect.width * dpr);
  canvas.height = Math.round(rect.height * dpr);
  const ctx = canvas.getContext("2d");
  ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
  return { ctx, width: rect.width, height: rect.height };
}

function drawChart(canvasId, points, title, unit) {
  const canvas = document.getElementById(canvasId);
  const { ctx, width: w, height: h } = resizeCanvas(canvas);

  ctx.clearRect(0, 0, w, h);

  const pad = { left: 55, right: 20, top: 25, bottom: 45 };

  const valid = points
    .map(p => ({
      t: p.timestamp,
      v: p.value === null || p.value === undefined ? null : Number(p.value)
    }))
    .filter(p => Number.isFinite(p.v));

  ctx.fillStyle = "#111827";
  ctx.font = "16px Arial";
  ctx.fillText(title, pad.left, 18);

  if (valid.length < 2) {
    ctx.fillStyle = "#6b7280";
    ctx.font = "14px Arial";
    ctx.fillText("No history yet", pad.left, h / 2);
    return;
  }

  const values = valid.map(p => p.v);
  let min = Math.min(...values);
  let max = Math.max(...values);

  if (min === max) {
    min -= 1;
    max += 1;
  } else {
    const margin = (max - min) * 0.12;
    min -= margin;
    max += margin;
  }

  const chartW = w - pad.left - pad.right;
  const chartH = h - pad.top - pad.bottom;

  // Grid
  ctx.strokeStyle = "#e5e7eb";
  ctx.lineWidth = 1;
  ctx.font = "12px Arial";
  ctx.fillStyle = "#6b7280";

  for (let i = 0; i <= 4; i++) {
    const y = pad.top + (chartH * i) / 4;
    ctx.beginPath();
    ctx.moveTo(pad.left, y);
    ctx.lineTo(w - pad.right, y);
    ctx.stroke();

    const val = max - ((max - min) * i) / 4;
    ctx.fillText(val.toFixed(1), 6, y + 4);
  }

  // X-axis labels
  const first = valid[0].t;
  const middle = valid[Math.floor(valid.length / 2)].t;
  const last = valid[valid.length - 1].t;

  ctx.fillText(new Date(first).toLocaleTimeString(), pad.left, h - 18);
  ctx.fillText(new Date(middle).toLocaleTimeString(), pad.left + chartW / 2 - 30, h - 18);
  ctx.fillText(new Date(last).toLocaleTimeString(), w - pad.right - 90, h - 18);

  // Line
  ctx.strokeStyle = "#2563eb";
  ctx.lineWidth = 2.5;
  ctx.beginPath();

  valid.forEach((p, index) => {
    const x = pad.left + (chartW * index) / (valid.length - 1);
    const y = pad.top + (max - p.v) * (chartH / (max - min));

    if (index === 0) ctx.moveTo(x, y);
    else ctx.lineTo(x, y);
  });

  ctx.stroke();

  // Points
  ctx.fillStyle = "#2563eb";
  valid.forEach((p, index) => {
    const x = pad.left + (chartW * index) / (valid.length - 1);
    const y = pad.top + (max - p.v) * (chartH / (max - min));
    ctx.beginPath();
    ctx.arc(x, y, 2.8, 0, Math.PI * 2);
    ctx.fill();
  });

  // Y axis title
  ctx.fillStyle = "#6b7280";
  ctx.fillText(unit, 6, 18);
}

function updateCards(latest) {
  document.getElementById("temperatureValue").textContent = formatValue(latest.temperature, "°C");
  document.getElementById("humidityValue").textContent = formatValue(latest.humidity, "%");
  document.getElementById("windSpeedValue").textContent = formatValue(latest.windSpeed, "km/h");
  document.getElementById("rainfallValue").textContent = formatValue(latest.rainfall, "mm");

  document.getElementById("connectionStatus").textContent = "Live";
  document.getElementById("lastUpdated").textContent = "Last update: " + formatTime(latest.timestamp);
}

function drawAllCharts() {
  const tempPoints = historyData.map(r => ({ timestamp: r.timestamp, value: r.temperature }));
  const humidityPoints = historyData.map(r => ({ timestamp: r.timestamp, value: r.humidity }));
  const windPoints = historyData.map(r => ({ timestamp: r.timestamp, value: r.windSpeed }));
  const rainPoints = historyData.map(r => ({ timestamp: r.timestamp, value: r.rainfall }));

  drawChart("tempChart", tempPoints, "Temperature Trend", "°C");
  drawChart("humidityChart", humidityPoints, "Humidity Trend", "%");
  drawChart("windChart", windPoints, "Wind Speed Trend", "km/h");
  drawChart("rainChart", rainPoints, "Rainfall Trend", "mm");
}

async function loadLatest() {
  try {
    const res = await fetch("/api/latest");
    const data = await res.json();

    if (data && Object.keys(data).length > 0) {
      updateCards(data);
    }
  } catch (err) {
    document.getElementById("connectionStatus").textContent = "Disconnected";
  }
}

async function loadHistory() {
  try {
    const res = await fetch(`/api/history?limit=${historyLimit}`);
    historyData = await res.json();
    drawAllCharts();
  } catch (err) {
    console.error("Failed to load history", err);
  }
}

async function refresh() {
  await loadLatest();
  await loadHistory();
}

refresh();
setInterval(refresh, 2000);
window.addEventListener("resize", drawAllCharts);