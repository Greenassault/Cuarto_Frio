// Lógica común de las gráficas de temperatura y humedad (página "Gráfica" y "Ver estadísticas").
// Los dos lugares dibujan igual: un punto por intervalo con el promedio, eje vertical
// ajustado a los datos, huecos sin línea y botones de variable y de sección.
// Necesita Chart.js cargado antes.
const GraficaComun = (() => {
  const COLORES = { 1: "#38bdf8", 2: "#fbbf24" };

  // "300" -> "5 minutos", "7200" -> "2 horas"
  function textoIntervalo(seg) {
    const plural = (n, unidad) => n + " " + unidad + (n === 1 ? "" : "s");
    if (seg < 3600) return plural(Math.round(seg / 60), "minuto");
    if (seg < 86400) return plural(Math.round(seg / 3600), "hora");
    return plural(Math.round(seg / 86400), "día");
  }

  // Puntos del servidor [fecha, n, temp_1, hum_1, temp_2, hum_2] -> {x, n, s}
  // x es la hora en milisegundos; s son las sumas (promedio * n), para poder seguir
  // sumando lecturas nuevas en vivo.
  function puntosDeServidor(lista) {
    return lista.map(([fecha, n, t1, h1, t2, h2]) => ({
      x: Date.parse(fecha), n: n, s: [t1 * n, h1 * n, t2 * n, h2 * n]
    }));
  }

  // Posición del dato dentro de s: temp_1, hum_1, temp_2, hum_2
  function indiceDato(numero, variable) {
    return (numero - 1) * 2 + (variable === "temp" ? 0 : 1);
  }

  // Puntos ordenados por hora -> puntos de la línea. Donde hayCorte(anterior, actual)
  // sea verdadero se deja un hueco (y: null) en vez de unir con una recta.
  function serie(puntos, indice, hayCorte) {
    const resultado = [];
    let previo = null;
    for (const p of puntos) {
      if (previo !== null && hayCorte(previo, p.x)) {
        resultado.push({ x: (previo + p.x) / 2, y: null });
      }
      resultado.push({ x: p.x, y: p.s[indice] / p.n });
      previo = p.x;
    }
    return resultado;
  }

  function crear(canvas, opciones) {
    return new Chart(canvas, {
      type: "line",
      data: { datasets: [] },
      options: {
        responsive: true,
        maintainAspectRatio: false,
        animation: false,
        interaction: { mode: "nearest", axis: "x", intersect: false },
        scales: {
          x: {
            type: "linear",
            ticks: { color: "#94a3b8", maxTicksLimit: 8, callback: opciones.formatoEjeX },
            grid: { color: "#334155" }
          },
          y: {
            title: { display: true, text: "°C", color: "#94a3b8" },
            ticks: { color: "#94a3b8" },
            grid: { color: "#334155" }
          }
        },
        plugins: {
          legend: { labels: { color: "#e2e8f0" } },
          tooltip: {
            callbacks: {
              title: (items) => opciones.tituloTooltip(items[0].parsed.x),
              label: (item) => item.dataset.label + ": " + item.parsed.y.toFixed(1) + " " + opciones.unidad()
            }
          }
        }
      },
      plugins: opciones.plugins || []
    });
  }

  // El eje vertical se ajusta a los datos que se ven, con un pequeño margen
  function ajustarEjeY(grafica) {
    const y = grafica.options.scales.y;
    const valores = grafica.data.datasets.flatMap((s) => s.data.map((p) => p.y).filter((v) => v !== null));
    if (valores.length === 0) {
      y.min = undefined;
      y.max = undefined;
      return;
    }
    const min = Math.min(...valores);
    const max = Math.max(...valores);
    const margen = Math.max((max - min) * 0.1, 0.5);
    y.min = min - margen;
    y.max = max + margen;
  }

  // Dibuja lo elegido en los botones. seccion: 0 = ambas, o el número de la sección.
  // d: { nombres, seccion, variable, puntos, hayCorte, xMin, xMax }
  function dibujar(grafica, d) {
    const numeros = Object.keys(d.nombres).map(Number);
    const visibles = d.seccion === 0 ? numeros : [d.seccion];
    grafica.data.datasets = visibles.map((numero) => ({
      label: d.nombres[numero],
      data: serie(d.puntos, indiceDato(numero, d.variable), d.hayCorte),
      borderColor: COLORES[numero],
      backgroundColor: COLORES[numero],
      borderWidth: 2,
      pointRadius: 1.5,
      pointHoverRadius: 4,
      cubicInterpolationMode: "monotone",   // suaviza sin pasarse de los valores reales
      spanGaps: false
    }));
    grafica.options.scales.y.title.text = d.variable === "temp" ? "°C" : "%";
    grafica.options.scales.x.min = d.xMin;
    grafica.options.scales.x.max = d.xMax;
    ajustarEjeY(grafica);
    grafica.update();
  }

  // Un grupo de botones donde solo uno está elegido. alElegir(boton) guarda la
  // elección y luego se vuelve a dibujar con alCambiar().
  function conectarBotones(selector, alElegir, alCambiar) {
    const botones = document.querySelectorAll(selector + " button");
    botones.forEach((b) => {
      b.addEventListener("click", () => {
        botones.forEach((x) => {
          x.classList.remove("activo");
          x.setAttribute("aria-pressed", "false");
        });
        b.classList.add("activo");
        b.setAttribute("aria-pressed", "true");
        alElegir(b);
        alCambiar();
      });
    });
  }

  return { COLORES, textoIntervalo, puntosDeServidor, crear, dibujar, conectarBotones };
})();
