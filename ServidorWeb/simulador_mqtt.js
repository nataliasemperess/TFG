const clientSimulator = mqtt.connect('ws://localhost:9001');


clientSimulator.on('connect', () => {
  setInterval(() => {
    const randomFloor = Math.floor( Math.random() * 5); // Planta 0 a 5
    const doorState = Math.random() > 0.5 ? 'OPEN' : 'CLOSED';
    const movementState = Math.random() > 0.5 ? 'MOVING' : 'STOPPED';

    clientSimulator.publish('ascensor/planta', randomFloor.toString());
    clientSimulator.publish('ascensor/puerta', doorState);
    clientSimulator.publish('ascensor/movimiento', movementState);
  }, 500);
});

