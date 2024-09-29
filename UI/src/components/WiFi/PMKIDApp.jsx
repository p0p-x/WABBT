import { useState } from 'react';
import {
  useNavigate,
} from "react-router-dom";

import Grid from '@mui/material/Grid';
import Button from '@mui/material/Button';
import Typography from '@mui/material/Typography';

import {
  sortDevs,
  Device,
  BadgeApp,
  BadgeAppContext
} from '../common';

const app = {
  id: 'pmkid',
  opt: 11,
  name: 'PMKID',
  desc: `
  Collects PMKIDs to
  later be cracked
  on a more powerful
  device. Either send
  deauth packets or try
  to connect to target AP
  with another device once
  running.
`
};

export const PMKIDApp = () => {
  const navigate = useNavigate();
  const [ap, setAP] = useState(false);
  
  const onBack = (e) => {
    navigate('/wifi');
  };

  const scan = () => {
    if (window.socket?.readyState === 1) {
      window.socket.send(JSON.stringify({
        opt: app.opt,
        action: "scan",
      }));
    }
  };

  const selectAP = (dev, setState) => {
    setAP(dev);
    setState(p => p+=1);
  };

  const startAttack = (ap_id) => {
    if (window.socket?.readyState === 1) {
      window.onOptSuccess = () => setTimeout(() => {
        window.location.reload()
        window.onOptSuccess = false;
      }, 1000);
      window.socket.send(JSON.stringify({
        opt: app.opt, // get gattattack scan
        action: "start",
        ap_id
      }));
    }
  };

  const deauth = () => {
    if (window.socket?.readyState === 1) {
      window.socket.send(JSON.stringify({
        opt: app.opt,
        action: "deauth"
      }));
    }
  };

  return (
    <BadgeApp
      app={app}
      startFunc={scan}
      onBack={onBack}
    >
      <BadgeAppContext.Consumer>
        {({ state, results, setState }) => (
          <>
            {state === 1 && (
              <Typography variant='body1'>Scanning APS...</Typography>
            )}

            {state === 2 && results && (
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

            {state === 3 && (
              <Button
                type='submit'
                variant='contained'
                color='primary'
                onClick={() => startAttack(ap.id)}
              >
                Start
              </Button>
            )}

            {state === 4 && (
              <Button
                type='submit'
                variant='contained'
                color='primary'
                onClick={deauth}
              >
                Send Deauth
              </Button>
            )}
          </>
        )}
      </BadgeAppContext.Consumer>
    </BadgeApp>
  );
};

PMKIDApp.propTypes = {};

export default PMKIDApp;
