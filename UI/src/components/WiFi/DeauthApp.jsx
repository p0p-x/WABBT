import { useState } from 'react';
import {
  useNavigate,
} from "react-router-dom";

import Grid from '@mui/material/Grid';
import Typography from '@mui/material/Typography';

import {
  sortDevs,
  Device,
  BadgeApp,
  BadgeAppContext,
} from '../common';

const app = {
  id: 'deauth',
  opt: 1,
  name: 'Deauth',
  desc: `
  Deauthenticates WIFI
  clients connected
  to networks. Can
  either target single
  stations or whole AP.
`
};

export const DeauthApp = () => {
  const navigate = useNavigate();
  const [ap, setAP] = useState(false);

  const onBack = (state, setState) => {
    if (state === 5) {
      setState(s => s-=1);
    } else {
      navigate('/wifi');
    }
  };

  const scan = () => {
    if (window.socket?.readyState === 1) {
      window.socket.send(JSON.stringify({
        opt: app.opt, // deauth scan for aps
        action: "scan",
      }));
    }
  };

  const selectAP = (dev, setState) => {
    setAP(dev);
    setState((p) => p+=1);
  };

  const stopAttack = () => {
    if (window.socket?.readyState === 1) {
      if (confirm('r u sure?')) {
        window.socket.send(JSON.stringify({
          opt: app.opt, // get gattattack scan
          action: "stop"
        }));
      }
    }
  }

  const startAttack = (ap_id, sta_id, reason = 0x01) => {
    if (window.socket?.readyState === 1) {
      if (confirm('Continue? AP will disconnect and you will have to manually stop this attack and reconnect to AP')) {
        window.socket.send(JSON.stringify({
          opt: app.opt, // get gattattack scan
          action: "start",
          ap_id,
          sta_id,
          reason
        }));
      }
    }
  };

  return (
    <BadgeApp
      app={app}
      startFunc={scan}
      stopFunc={stopAttack}
      onBack={onBack}
    >
      <BadgeAppContext.Consumer>
        {({ state, results, setState }) => (
          <>
            {state === 2 && (
              <Typography variant='body1'>Scanning APS...</Typography>
            )}

            {state === 3 && (
              <Typography variant='body1'>Scanning Stations...</Typography>
            )}

            {state === 4 && results && (
              <>
                <Typography variant='body1' sx={{ mb: 2 }}>
                  APS: {results.length}
                </Typography>

                <Grid container spacing={2}>
                  {results.sort(sortDevs).map((r) => (
                    <Grid item xs={12} sm={6} md={4} lg={3} key={r.id}>
                      <Device
                        dev={r}
                        onClick={() => selectAP(r, setState)}
                      />
                    </Grid>
                  ))}
                </Grid>
              </>
            )}

            {state === 5 && (
              <>
                <Typography variant='body1' sx={{ mb: 2 }}>
                  Select AP or single STA
                </Typography>
                <Grid container spacing={2}>
                  <Grid item xs={12} lg={3} md={4}>
                    <Typography variant='h4' sx={{ mb: 2 }}>
                      AP
                    </Typography>
                    <Device
                      dev={ap}
                      onClick={() => startAttack(ap.id, -1)}
                    />
                  </Grid>
                  <Grid item xs={12} lg={9} md={8}>
                    <Typography variant='h4' sx={{ mb: 2 }}>
                      Stations ({ap.stations?.length ?? 0})
                    </Typography>
                    <Grid container spacing={3}>
                      {ap.stations?.map((r) => (
                        <Grid item xs={12} sm={6} md={6} lg={4} key={r.id}>
                          <Device
                            dev={r}
                            onClick={() => startAttack(ap.id, r.id)}
                          />
                        </Grid>
                      ))}
                    </Grid>
                  </Grid>
                </Grid>
              </>
            )}    
          </>
        )}
      </BadgeAppContext.Consumer>
    </BadgeApp>
  );
};

DeauthApp.propTypes = {};

export default DeauthApp;
