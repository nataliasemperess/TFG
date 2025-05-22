// ------------------------------------
// ELEMENTOS DEL DOM
// ------------------------------------
const elevatorCab = document.getElementById('elevatorCab');
const elevatorCabPanel = document.getElementById('elevatorCabPanel');  // Panel derecha

const movementStatus = document.getElementById('movementStatus');
const doorStatus = document.getElementById('doorStatus');
const movementImage = document.getElementById('movementImage');
const doorImage = document.getElementById('doorImage');

const floorStatus = document.getElementById('floorStatus');

const grafanaIframes = document.querySelectorAll('iframe');
const slides = document.querySelectorAll(".slides");
const tabButtons = document.querySelectorAll(".tablinks");


// ------------------------------------
// PRUEBA
// ------------------------------------
let activeIndex = 0;
let maxActiveIndex = 5;
let inTransition = false;

document.addEventListener("wheel", (event) => {
  if (event.deltaY < 0) {
    if (!inTransition && activeIndex > 0) {
      activeIndex--;
    }
  } else {
    if (!inTransition && activeIndex < maxActiveIndex) {
      activeIndex++;

    }
  }
  showSlide(activeIndex);
});

document.addEventListener("keydown", function (event) {
  if (inTransition) return;

  if (event.key === "ArrowRight" || event.key === "ArrowUp") {
    if (activeIndex < maxActiveIndex) {
      activeIndex++;
      showSlide(activeIndex);
    }
  } else if (event.key === "ArrowLeft" || event.key === "ArrowDown") {
    if (activeIndex > 0) {
      activeIndex--;
      showSlide(activeIndex);
    }
  }
});

function startTransition() {
  inTransition = true;
  setTimeout(() => {
    inTransition = false;
  }, 500);
}

function showSlide(index) {
  slides.forEach((slide, i) => {
    slide.style.display = (i === index) ? "block" : "none";
  });
  tabButtons.forEach((btn, i) => {
    btn.classList.toggle("active", i === index);
  });
  activeIndex = index;
}

// Click en pestañas
tabButtons.forEach((btn, index) => {
  btn.addEventListener("click", (evt) => {
    showSlide(index);
  });
});


window.onload = function() {
  // Ocultar el loading screen después de cargar los iframes y el contenido
  document.getElementById('loading-screen').style.display = 'none';

  // Asegúrate de que las slides estén listas
  setSlides();
};

function setSlides() {
  const divs = ["div-1", "div-2", "div-3", "div-4", "div-5", "div-6"];
  divs.forEach((div, index) => {
    const divElement = document.getElementsByClassName(div)[0];

    divElement.classList.remove("active-slide", "inactive-slide-down", "inactive-slide-up");
    if (index < activeIndex) {
      divElement.classList.add("inactive-slide-up");
    } else if (index > activeIndex) {
      divElement.classList.add("inactive-slide-down");
    } else {
      divElement.classList.add("active-slide");
    }
  });
  // Actualiza los dots
  const dots = document.querySelectorAll('.dot');
  dots.forEach((dot, index) => {
    dot.classList.toggle('active-dot', index === activeIndex);
  });
}
// Hacer clic en los dots para navegar
document.querySelectorAll('.dot').forEach(dot => {
  dot.addEventListener('click', (event) => {
    const index = parseInt(event.target.getAttribute('data-index'));
    if (!inTransition && index !== activeIndex) {
      activeIndex = index;
      startTransition();
      setSlides();
    }
  });
});


// ------------------------------------
// CONEXIÓN MQTT
// ------------------------------------
const client = mqtt.connect('ws://localhost:9001');
let isMoving = false;

client.on('connect', () => {
  client.subscribe('ascensor/planta');
  client.subscribe('ascensor/puerta');
  client.subscribe('ascensor/movimiento');
});

// ------------------------------------aa
// RECEPCIÓN DE MENSAJES MQTT
// ------------------------------------
client.on('message', (topic, message) => { 
  const value = message.toString();

  switch (topic) {
    case 'ascensor/planta':
      actualizarPlanta(parseInt(value));
      break;

    case 'ascensor/movimiento':
      actualizarMovimiento(value);
      break;

    case 'ascensor/puerta':
      actualizarPuerta(value);
      break;
  }
});

// ------------------------------------
// FUNCIONES DE ACTUALIZACIÓN
// ------------------------------------

let plantaAnterior = null;
let ultimaParadaTimestamp = null;
let direccionActual = "---";



function actualizarPlanta(floor) {
  if (floor < 0) floor = 0;
  if (floor > 5) floor = 5;

  if (plantaAnterior !== null && isMoving) {
    if (floor > plantaAnterior) {
      movementImage.src = 'img/subida.png';
      direccionActual = 'Going up';
    } else if (floor < plantaAnterior) {
      movementImage.src = 'img/bajada.png';
      direccionActual = 'Going down';
    }
  } else if (!isMoving) {
    movementImage.src = 'img/parado_transparente.png';
    direccionActual = '---';
    ultimaParadaTimestamp = Date.now();
  }

  plantaAnterior = floor;
  floorStatus.textContent = `${floor}`;

  const posiciones = {
    0: 0,
    1: 165,
    2: 285,
    3: 390,
    4: 494,
    5: 590
  };

  // Sincroniza ambas cabinas
  elevatorCab.style.bottom = `${posiciones[floor]}px`;
  elevatorCabPanel.style.bottom = `${posiciones[floor]}px`;

  actualizarPanelEstado();
}


function actualizarMovimiento(value) {
  isMoving = (value === 'MOVING');
  document.getElementById("estadoActual").textContent = isMoving ? "Moving" : "Stopped";
}

function actualizarPuerta(value) {
  document.getElementById("estadoPuerta").textContent = (value === 'OPEN') ? "Opened" : "Closed";

  if (doorImage) {
    doorImage.src = value === 'OPEN' ? "img/open.png" : "img/closed.png";
  }

  if (value === 'OPEN' && !isMoving) {
    setTimeout(abrirPuertas, 2000);
  } else if (value === 'CLOSED') {
    cerrarPuertas();
  }
}

function actualizarPanelEstado() {
  document.getElementById("direccionActual").textContent = direccionActual;

  if (ultimaParadaTimestamp) {
    const segundos = Math.floor((Date.now() - ultimaParadaTimestamp) / 1000);
    const minutos = Math.floor(segundos / 60);
    const resto = segundos % 60;
    document.getElementById("tiempoParada").textContent =
      `${minutos.toString().padStart(2, '0')}:${resto.toString().padStart(2, '0')}`;
  }
}
setInterval(() => {
  if (!isMoving) actualizarPanelEstado();
}, 1000);



// ------------------------------------
// GESTIÓN DE GRAFANA
// ------------------------------------
document.querySelectorAll('.time-button').forEach(button => {
  button.addEventListener('click', () => {

    // Elimina "active" de todos los botones
    document.querySelectorAll('.time-button').forEach(btn => btn.classList.remove('active'));

    // Añade "active" al botón pulsado
    button.classList.add('active');

    const selectedValue = button.getAttribute('data-range');

    const rangeMap = {
      '1': 'now-1m',
      '720': 'now-12h',
      '1440': 'now-24h',
      '10080': 'now-7d',
      '43200': 'now-30d',
      '525600': 'now-1y'
    };

    const selectedFrom = rangeMap[selectedValue] || 'now-7d';
    const selectedTo = 'now';

    grafanaIframes.forEach((iframe) => {
      const url = new URL(iframe.src);
      url.searchParams.set('from', selectedFrom);
      url.searchParams.set('to', selectedTo);
      iframe.src = url.toString();
    });
  });
});

// ------------------------------------
// FUNCIONES DE ANIMACIÓN DE PUERTAS
// ------------------------------------
function abrirPuertas() {
  elevatorCab.src = 'img/prueba_def.png';
  elevatorCabPanel.src = 'img/prueba_def.png';
}

function cerrarPuertas() {
  elevatorCab.src = 'img/dark_lift.png';
  elevatorCabPanel.src = 'img/dark_lift.png';
}


// TABS
function openSlide(evt, slideName) {
  var i, slides, tablinks;
  slides = document.getElementsByClassName("slides");
  for (i = 0; i < slides.length; i++) {
    slides[i].style.display = "none";
  }
  tablinks = document.getElementsByClassName("tablinks");
  for (i = 0; i < tablinks.length; i++) {
    tablinks[i].className = tablinks[i].className.replace(" active", "");
  }
  document.querySelector('.' + slideName).style.display = "block";
  evt.currentTarget.className += " active";
}

// Abrir la primera pestaña por defecto
window.onload = function () {
  document.getElementById('loading-screen').style.display = 'none';
  document.querySelector(".tablinks").click();
};
