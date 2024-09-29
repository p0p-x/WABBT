import { useState } from 'react';
import { useNavigate } from "react-router-dom";

import Grid from '@mui/material/Grid';
import TextField from '@mui/material/TextField';

import { BadgeApp, BadgeAppContext } from '../common';

const app = {
  id: 'drone',
  opt: 13,
  name: 'Fake Drone',
  class: 'defensive',
  color: 'error',
  desc: `
  Sends OpenDroneID BLE
  advertisements to create
  a fake drone that flies
  around given coords.
`};

export const DroneApp = () => {
  const navigate = useNavigate();
  const [name, setName] = useState('HackThePlan8');
  const [lat, setLat] = useState('42.963795');
  const [lng, setLng] = useState('-85.670006');

  const startDrone = (e) => {
    e.preventDefault();
    if (window.socket?.readyState === 1) {
      window.socket.send(JSON.stringify({
        opt: app.opt,
        action: "start",
        name,
        lat,
        lng
      }));
    }
  }

  return (  
    <BadgeApp
      app={app}
      startFunc={startDrone}
      onBack={() => navigate('/ble')}
    >
      <BadgeAppContext.Consumer>
        {({ state }) => (
          <>
            {state === 0 && (
              <form onSubmit={(e) => e.preventDefault()}>
                <Grid container spacing={2} sx={{ mt: 2 }}>
                  <Grid item xs={12}>
                    <TextField
                      sx={{ mr: 2 }}
                      value={lat}
                      onChange={(e) => setLat(e.target.value)}
                      size='small'
                      label='Latitude'
                      placeholder='Latitude -52'
                    />
                    <TextField
                      value={lng}
                      onChange={(e) => setLng(e.target.value)}
                      size='small'
                      label='Longitude'
                      placeholder='Longitude 0.719'
                    />
                  </Grid>
                  <Grid item>
                    <TextField
                      value={name}
                      onChange={(e) => setName(e.target.value)}
                      size='small'
                      label='Drone Name'
                      placeholder='Drone Name'
                    />
                  </Grid>
                </Grid>
              </form>
            )}
          </>
        )}
      </BadgeAppContext.Consumer>
    </BadgeApp>
  );
};

export default DroneApp;
