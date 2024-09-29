import { useState} from 'react';

import { useNavigate } from 'react-router-dom';

import Box from '@mui/material/Box';
import Grid from '@mui/material/Grid';
import Button from '@mui/material/Button';
import Typography from '@mui/material/Typography';

import {
  Device,
  sortDevs,
  BadgeApp,
  BadgeAppContext
} from '../common';

const app = {
  id: 'blereplay',
  opt: 15,
  name: 'BLE Replay',
  class: 'defensive',
  color: 'primary',
  desc: `
  Replays BLE GATT 
  captures.
`
};

export const ReplayApp = () => {
  const navigate = useNavigate();
  const [dev, setDev] = useState(false);
  const [packets, setPackets] = useState('');

  const stop = () => {
    if (window.socket?.readyState === 1) {
      if (confirm('r u sure?')) {
        window.socket.send(JSON.stringify({
          opt: app.opt,
          action: "stop"
        }));
      }
    }
  };

  const start = () => {
    if (window.socket?.readyState === 1) {
      if (confirm('r u sure?')) {
        window.socket.send(JSON.stringify({
          opt: app.opt,
          action: "start",
          id: dev.id,
          packets: JSON.parse(packets),
        }));
      }
    }
  };

  const scanDevices = () => {
    if (window.socket?.readyState === 1) {
      window.socket.send(JSON.stringify({
        opt: app.opt,
        action: "scan",
      }));
    }
  };

  const selectDevice = (d, setState) => {
    setDev(d);
    setState((s) => s += 1);
  };

  return (  
    <BadgeApp
      app={app}
      startFunc={scanDevices}
      stopFunc={stop}
      onBack={() => navigate('/ble')}
    >
      <BadgeAppContext.Consumer>
        {({ state, setState, results }) => (
          <>
            {state === 1 && (
              <Typography variant='body1'>
                scanning for devices...
              </Typography>
            )}

            {state === 2 && results && (
              <>
                <Typography variant='body1' sx={{ mb: 2 }}>
                  Devices: {results.length}
                </Typography>

                <Grid container spacing={3}>
                  {results.sort(sortDevs).map((r) => (
                    <Grid item xs={12} sm={6} md={4} lg={3} key={r.id}>
                      <Device
                        dev={r}
                        onClick={() => selectDevice(r, setState)}
                      />
                    </Grid>
                  ))}
                </Grid>
              </>
            )}

            {state === 3 && dev && (
              <>
                <Box
                  component='textarea'
                  sx={{ display: 'block', width: '100%', height: '300px' }}
                  placeholder='Paste Log Here'
                  value={packets}
                  onChange={(e) => setPackets(e.target.value)}
                />
                <Button
                  sx={{ mt: 5 }}
                  variant='contained'
                  color='primary'
                  size='medium'
                  onClick={start}
                >
                  Send
                </Button>
              </>
            )}
        </>
      )}
      </BadgeAppContext.Consumer>
    </BadgeApp>
  );
};

export default ReplayApp;
