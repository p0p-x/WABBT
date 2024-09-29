import { useNavigate } from 'react-router-dom';

import Grid from '@mui/material/Grid';
import Typography from '@mui/material/Typography';

import { Device, BadgeApp, BadgeAppContext } from '../common';

const app = {
  id: 'gatk',
  opt: 3,
  class: 'offensive',
  color: 'error',
  name: 'Gatt Attack',
  desc: `
  Spoofs a BLE device. This will clone
  MAC and GATT services and advertise at a fast
  interval tricking devices to connect. Then
  communincation between can be viewed.`
};

export const GattAttackApp = ({
}) => {
  const navigate = useNavigate();

  const stopAttack = () => {
    if (window.socket?.readyState === 1) {
      if (confirm('r u sure?')) {
        window.socket.send(JSON.stringify({
          opt: app.opt, // get gattattack scan
          action: "stop"
        }));
      }
    }
  };

  const startAttack = (dev) => {
    if (window.socket?.readyState === 1) {
      if (confirm('r u sure?')) {
        window.socket.send(JSON.stringify({
          opt: app.opt, // get gattattack scan
          action: "start",
          id: dev.id,
        }));
      }
    }
  };

  const scanDevices = () => {
    if (window.socket?.readyState === 1) {
      window.socket.send(JSON.stringify({
        opt: app.opt, // get gattattack scan
        action: "scan",
      }));
    }
  };

  return (
    <BadgeApp
      app={app}
      startFunc={scanDevices}
      stopFunc={stopAttack}
      onBack={() => navigate('/ble')}
    >
      <BadgeAppContext.Consumer>
        {({ state, results }) => (
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
                  {results.sort((a, b) => b.rssi - a.rssi).map((r) => (
                    <Grid item xs={12} sm={6} md={4} lg={3} key={r.id}>
                      <Device
                        dev={r}
                        onClick={() => startAttack(r)}
                      />
                    </Grid>
                  ))}
                </Grid>
              </>
            )}
        </>
      )}
      </BadgeAppContext.Consumer>
    </BadgeApp>
  );
};

GattAttackApp.propTypes = {};

export default GattAttackApp;
