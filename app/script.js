async function fetchSensorData() {
  try {
    const response = await fetch("https://bzju8opnek.execute-api.us-east-1.amazonaws.com/dev/api/data");
    if (!response.ok) throw new Error("Failed to fetch sensor data");
    return await response.json();
  } catch (error) {
    console.error("Error fetching sensor data:", error);
    return [];
  }
}

document.addEventListener("DOMContentLoaded", async () => {
  const sensorData = await fetchSensorData();

  if (!sensorData || sensorData.length === 0) return;

  const latest = sensorData[0];

  function updateText(id, text) {
    const el = document.getElementById(id);
    if (el) el.textContent = text;
  }

  // Calculate storage percent
  const storagePercent = (latest.angle3 !== undefined ? latest.angle3 * 100 : (latest.coffee_storage_percent ?? 80));

  updateText("temp_main", `${latest.temperature_celsius}°C`);
  updateText("hum_main", `${latest.humidity_air_rh.toFixed(1)}%`);
  updateText("soil_humidity_main", `${(latest.humidity_soil_percent).toFixed(1)}%`);
  updateText("storage_main", `${storagePercent.toFixed(1)}%`);
  updateText("storage_box", `${storagePercent.toFixed(1)}%`);
  updateText("container_status", latest.angle1 === 2.25 ? "Closed" : "Open");

  // ✅ FIXED: Use ISO timestamp field
  updateText("timestamp", new Date(latest.timestamp).toLocaleString("pt-PT", { timeZone: "Europe/Lisbon" }));

  const alerts = [];

  if (latest.humidity_air_rh > 60) {
    alerts.push("High air humidity!");
  }

  if (latest.temperature_celsius <= 15 && latest.temperature_celsius > 5) {
    alerts.push("Normal temperature!");
  } else if (latest.temperature_celsius <= 5) {
    alerts.push("Low temperature!");
  } else if (latest.temperature_celsius > 15) {
    alerts.push("High temperature!");
  }

  if (latest.humidity_soil_percent > 13) {
    alerts.push("Risk of beans contamination due to low humidity.");
  } else {
    alerts.push("Normal beans humidity.");
  }

  if (storagePercent < 30) {
    alerts.push("Coffee storage low. Please refill.");
  }

  const alertsDiv = document.getElementById("alerts");
  const alertList = document.getElementById("alert_list");

  if (alertsDiv) {
    alertsDiv.innerHTML = alerts.length > 0 ? alerts.map(a => `- ${a}`).join("<br>") : "- No alerts";
  }

  if (alertList) {
    alertList.innerHTML = alerts.length > 0
      ? alerts.map(a => `<li>${a}</li>`).join("")
      : "<li>No alerts</li>";
  }

  const historyTableBody = document.querySelector("#history-table tbody");
  if (historyTableBody) {
    historyTableBody.innerHTML = "";

    sensorData.slice(1).forEach(entry => {
      const row = document.createElement("tr");
      row.innerHTML = `
        <td>${new Date(entry.timestamp).toLocaleString("pt-PT", { timeZone: "Europe/Lisbon" })}</td>
        <td>${entry.temperature_celsius}°C</td>
        <td>${entry.humidity_air_rh.toFixed(1)}%</td>
        <td>${(entry.humidity_soil_percent).toFixed(1)}%</td>
        <td>${entry.angle1 === 2.25 ? "Closed" : "Open"}</td>
        <td>${((entry.angle3 ?? entry.coffee_storage_percent ?? 0) * 100).toFixed(1)}%</td>
      `;
      historyTableBody.appendChild(row);
    });
  }

  // ------------------------------
  // CHARTS
  // ------------------------------
  function createLineChart(ctx, label, data, timeLabels, yLabel, color) {
    return new Chart(ctx, {
      type: "line",
      data: {
        labels: timeLabels,
        datasets: [{
          label: label,
          data: data,
          borderColor: color,
          backgroundColor: color + "33",
          fill: true,
          tension: 0.3,
          pointRadius: 4,
          pointHoverRadius: 6,
          borderWidth: 3,
        }]
      },
      options: {
        responsive: true,
        plugins: {
          legend: {
            labels: {
              color: "#111",
              font: { size: 20, weight: "bold" }
            }
          },
          tooltip: {
            bodyFont: { size: 14 },
            titleFont: { size: 20 }
          }
        },
        scales: {
          y: {
            beginAtZero: false,
            title: {
              display: true,
              text: yLabel,
              color: "#111",
              font: { size: 20, weight: "bold" }
            },
            ticks: {
              color: "#111",
              font: { size: 20 }
            }
          },
          x: {
            title: {
              display: true,
              text: "Time",
              color: "#111",
              font: { size: 20, weight: "bold" }
            },
            ticks: {
              color: "#111",
              font: { size: 20 }
            }
          }
        }
      }
    });
  }

  const tempCanvas = document.getElementById("tempChart");
  const humCanvas = document.getElementById("humChart");
  const soilHumCanvas = document.getElementById("soilHumChart");
  const storageCanvas = document.getElementById("storageChart");

  if (tempCanvas && humCanvas && soilHumCanvas && storageCanvas) {
    const reversed = [...sensorData].reverse();

    // ✅ FIXED: Use ISO timestamp field for chart time labels
    const timeLabels = reversed.map(d =>
      new Date(d.timestamp).toLocaleTimeString("pt-PT", { timeZone: "Europe/Lisbon" })
    );

    const temps = reversed.map(d => d.temperature_celsius);
    const hums = reversed.map(d => d.humidity_air_rh);
    const soilHums = reversed.map(d => d.humidity_soil_percent);
    const storage = reversed.map(d => d.angle3 !== undefined ? d.angle3 * 100 : (d.coffee_storage_percent ?? 0));

    createLineChart(tempCanvas.getContext("2d"), "Temperature (°C)", temps, timeLabels, "°C", "#cc0000");
    createLineChart(humCanvas.getContext("2d"), "Air Humidity (%)", hums, timeLabels, "%", "#003399");
    createLineChart(soilHumCanvas.getContext("2d"), "Beans Humidity (%)", soilHums, timeLabels, "%", "#006644");
    createLineChart(storageCanvas.getContext("2d"), "Coffee Storage (%)", storage, timeLabels, "%", "#aa7700");
  }
});
