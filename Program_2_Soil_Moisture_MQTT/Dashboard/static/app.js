let historyData = [];

const historyLimit = 300;

function formatValue(value, suffix) {
  if (value === null || value === undefined || Number.isNaN(value))
    return `-- ${suffix}`;

  return `${Number(value).toFixed(1)} ${suffix}`;
}

function formatTime(ts) {
  if (!ts) return "--";

  const d = new Date(ts);

  if (isNaN(d.getTime()))
    return ts;

  return d.toLocaleString();
}

function resizeCanvas(canvas) {

  const rect = canvas.getBoundingClientRect();
  const dpr = window.devicePixelRatio || 1;

  canvas.width = Math.round(rect.width * dpr);
  canvas.height = Math.round(rect.height * dpr);

  const ctx = canvas.getContext("2d");

  ctx.setTransform(
    dpr,0,0,dpr,0,0
  );

  return {
    ctx,
    width: rect.width,
    height: rect.height
  };
}

function drawChart(canvasId, points, title, unit) {

  const canvas =
    document.getElementById(canvasId);

  const { ctx, width: w, height: h } =
    resizeCanvas(canvas);

  ctx.clearRect(0,0,w,h);

  const pad = {
    left:55,
    right:20,
    top:25,
    bottom:45
  };

  const valid =
    points.filter(
      p => Number.isFinite(Number(p.value))
    );

  ctx.fillStyle = "#111827";
  ctx.font = "16px Arial";
  ctx.fillText(title,pad.left,18);

  if(valid.length < 2)
  {
    ctx.fillStyle="#6b7280";
    ctx.font="14px Arial";
    ctx.fillText(
      "Waiting for data...",
      pad.left,
      h/2
    );
    return;
  }

  const values =
    valid.map(p => Number(p.value));

  let min = Math.min(...values);
  let max = Math.max(...values);

  if(min === max)
  {
    min -= 1;
    max += 1;
  }

  const chartW =
    w-pad.left-pad.right;

  const chartH =
    h-pad.top-pad.bottom;

  ctx.strokeStyle="#e5e7eb";
  ctx.lineWidth=1;

  for(let i=0;i<=4;i++)
  {
    const y =
      pad.top +
      chartH*i/4;

    ctx.beginPath();
    ctx.moveTo(pad.left,y);
    ctx.lineTo(w-pad.right,y);
    ctx.stroke();
  }

  ctx.strokeStyle="#2563eb";
  ctx.lineWidth=3;
  ctx.beginPath();

  valid.forEach((p,index)=>{

    const x =
      pad.left +
      chartW*index/(valid.length-1);

    const y =
      pad.top +
      (max-p.value) *
      (chartH/(max-min));

    if(index===0)
      ctx.moveTo(x,y);
    else
      ctx.lineTo(x,y);

  });

  ctx.stroke();
}

function updateCards(data)
{
  document.getElementById(
    "currentValue"
  ).textContent =
    formatValue(data.current,"%");

  document.getElementById(
    "minValue"
  ).textContent =
    formatValue(data.min,"%");

  document.getElementById(
    "maxValue"
  ).textContent =
    formatValue(data.max,"%");

  document.getElementById(
    "avgValue"
  ).textContent =
    formatValue(data.avg,"%");

  document.getElementById(
    "connectionStatus"
  ).textContent =
    "Live";

  document.getElementById(
    "lastUpdated"
  ).textContent =
    "Last update: " +
    new Date().toLocaleTimeString();
}

function drawAllCharts()
{
  const soilPoints =
    historyData.map((v,i)=>({
      timestamp:i,
      value:v
    }));

  drawChart(
    "soilChart",
    soilPoints,
    "Soil Moisture Trend",
    "%"
  );
}

async function refresh()
{
  try {

    const response =
      await fetch("/api/data");

    const data =
      await response.json();

    updateCards(data);

    historyData =
      data.history || [];

    drawAllCharts();

  }
  catch(err){

    document.getElementById(
      "connectionStatus"
    ).textContent =
      "Disconnected";
  }
}

refresh();

setInterval(
  refresh,
  2000
);

window.addEventListener(
  "resize",
  drawAllCharts
);