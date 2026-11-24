// Lógica común de las gráficas de temperatura y humedad (página "Gráfica" y "Ver estadísticas").
// Los dos lugares dibujan igual: un punto por intervalo con el promedio, eje vertical
// ajustado a los datos, huecos sin línea y botones de variable y de sección.
// Cada sensor (Sección A, B, C, D) es una línea propia; las lecturas con ok falso
// no se usan. Necesita Chart.js cargado antes.
const GraficaComun = (() => {
  // Colores bien distintos sobre fondo oscuro. C y D además van punteadas para
  // poder distinguirlas aunque alguien no vea bien los colores.
  const COLORES = { 1: "#38bdf8", 2: "#fbbf24", 3: "#f472b6", 4: "#4ade80" };
  const PUNTEADO = { 1: [], 2: [], 3: [6, 4], 4: [6, 4] };

  // "300" -> "5 minutos", "7200" -> "2 horas"
  function textoIntervalo(seg) {
    const plural = (n, unidad) => n + " " + unidad + (n === 1 ? "" : "s");
    if (seg < 3600) return plural(Math.round(seg / 60), "minuto");
    if (seg < 86400) return plural(Math.round(seg / 3600), "hora");
    return plural(Math.round(seg / 86400), "día");
  }

  // Un intervalo de la gráfica: {x, n, temp, hum}. x es la hora en milisegundos.
  // Por sensor (posición 0 = sensor 1): n = lecturas buenas, temp y hum = sus sumas
  // (promedio * n), para poder seguir sumando lecturas nuevas en vivo.
  function grupoVacio(x, cantidad) {
    return { x: x, n: Array(cantidad).fill(0), temp: Array(cantidad).fill(0), hum: Array(cantidad).fill(0) };
  }

  // Suma una lectura en vivo (como llega de /api/stream/) a su intervalo.
  // Igual que en el servidor, de cada sensor solo cuentan las lecturas con ok verdadero.
  function sumarLectura(g, d) {
    g.n.forEach((_, i) => {
      const n = i + 1;
      if (d["ok_" + n] === true && d["temp_" + n] !== null && d["hum_" + n] !== null) {
        g.n[i] += 1;
        g.temp[i] += d["temp_" + n];
        g.hum[i] += d["hum_" + n];
      }
    });
  }

  // Puntos del servidor [fecha, [n_1..n_4], [temp_1..temp_4], [hum_1..hum_4]] -> grupos
  function puntosDeServidor(lista) {
    return lista.map(([fecha, n, temp, hum]) => ({
      x: Date.parse(fecha),
      n: n,
      temp: temp.map((v, i) => (v === null ? 0 : v * n[i])),
      hum: hum.map((v, i) => (v === null ? 0 : v * n[i]))
    }));
  }

  // Grupos ordenados por hora -> puntos de la línea de un sensor. Se deja un hueco
  // (y: null) donde hayCorte(anterior, actual) sea verdadero o donde ese sensor no
  // tuvo ninguna lectura buena en el intervalo.
  function serie(puntos, numero, variable, hayCorte) {
    const i = numero - 1;
    const resultado = [];
    let previo = null;
    for (const p of puntos) {
      if (previo !== null && hayCorte(previo, p.x)) {
        resultado.push({ x: (previo + p.x) / 2, y: null });
      }
      resultado.push({ x: p.x, y: p.n[i] ? p[variable][i] / p.n[i] : null });
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
          // La leyenda muestra un trozo de línea (sólida o punteada) del color de cada sensor
          legend: { labels: { color: "#e2e8f0", usePointStyle: true, pointStyle: "line", pointStyleWidth: 36 } },
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

  // Dibuja lo elegido en los botones. seccion: 0 = todas, o el número de la sección.
  // d: { nombres, seccion, variable, puntos, hayCorte, xMin, xMax }
  function dibujar(grafica, d) {
    const numeros = Object.keys(d.nombres).map(Number);
    const visibles = d.seccion === 0 ? numeros : [d.seccion];
    grafica.data.datasets = visibles.map((numero) => ({
      label: d.nombres[numero],
      data: serie(d.puntos, numero, d.variable, d.hayCorte),
      borderColor: COLORES[numero],
      backgroundColor: COLORES[numero],
      borderDash: PUNTEADO[numero],
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

  return { COLORES, textoIntervalo, grupoVacio, sumarLectura, puntosDeServidor, crear, dibujar, conectarBotones };
})();
