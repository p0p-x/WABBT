import PropTypes from 'prop-types';
import {
  useState,
  useEffect,
  createContext
} from 'react';

import Card from '@mui/material/Card';
import Chip from '@mui/material/Chip';
import Grid from '@mui/material/Grid';
import Button from '@mui/material/Button';
import Typography from '@mui/material/Typography';
import IconButton from '@mui/material/IconButton';
import ArrowBackIcon from '@mui/icons-material/ArrowBack';
import CardContent from '@mui/material/CardContent';
import CardActionArea from '@mui/material/CardActionArea';

export const BadgeAppContext = createContext();

export const BadgeApp = ({
  app,
  onBack,
  startFunc = false,
  stopFunc = false,
  socketCb = false,
  children
}) => {
  const [state, setState] = useState(-1);
  const [output, setOutput] = useState('');
  const [results, setResults] = useState(false);
  
  const stop = (e) => {
    if (stopFunc) {
      stopFunc(e);
      setState(0);
      setOutput('');
      setResults(false);
    } else {
      if (window.socket?.readyState === 1) {
        window.socket.send(JSON.stringify({
          opt: app.opt,
          action: "stop",
        }));
        setState(0);
        setOutput('');
        setResults(false);
      }
    }
  }
  
  const start = (e) => {
    if (startFunc) {
      startFunc(e);
      setOutput('running');
      setState(1);
    } else {
      e.preventDefault();
      if (window.socket?.readyState === 1) {
        window.socket.send(JSON.stringify({
          opt: app.opt,
          action: "start"
        }));
        setOutput('running');
        setState(1);
      }
    }
  };

  window.socketCallback = (data) => {
    if (socketCb) {
      return socketCb(data);
    }

    if (data.app) {
      if (data.app !== app.opt) {
        // update for dif app id
        return false;
      }
    }

    if (data.connected) {
      setup();
    }

    if (data.state > -1) {
      setState(data.state);

      // gatt attack results
      if (app.opt !== 1 && data.state === 2 && !data.scan_done && !results) {
        window.socket.send(JSON.stringify({
          opt: app.opt,
          action: "results",
        }));
      }

      // deauth results
      if (data.state === 4 && !results) {
        window.socket.send(JSON.stringify({
          opt: app.opt, // get gattattack scan results
          action: "results",
        }));
      }
    }

    if (data.scan_done || data.update) {
      window.socket.send(JSON.stringify({
        opt: app.opt, // get deauth scan results
        action: "results",
      }));
    }

    if (data.results) {
      setResults(data.results);
    }

    if (data.output) {
      setOutput((o) => `${o}\n${data.output}`);
    }
  }

  const setup = () => {
    try {
      if (window.socket?.readyState === 1) {
        window.socket.send(JSON.stringify({
          opt: app.opt, // get app status
          action: "status",
        }));
      } else {
        setTimeout(setup, 2000);
      }
    } catch (err) {
      console.error(err);
    }
  }

  useEffect(() => {
    setup();
  }, []);

  return (
    <>
      <IconButton onClick={() => onBack(state, setState)}>
        <ArrowBackIcon />
      </IconButton>
      <span>Back</span>

      <Typography variant='h3'>
        {state >= 1 && (
          <Chip
            sx={{ mr: 1 }}
            label='Running'
            variant='outlined'
            color='primary'
          />
        )}
        {app.name}
      </Typography>
      <Typography sx={{ mb: 5 }} variant='body1'>{app.desc}</Typography>

      {state > 0 && (
        <>
          <Button
            type='submit'
            sx={{ mb: 5 }}
            variant='contained'
            color='primary'
            onClick={stop}
          >
            Stop
          </Button>

          <pre>{output}</pre>
        </>
      )}

      {state === 0 && (
        <Button
          type='submit'
          variant='contained'
          color='primary'
          onClick={start}
        >
          Start
        </Button>
      )}

      <BadgeAppContext.Provider
        value={{
          state,
          output,
          results,
          setState,
          setOutput,
        }}
      >
        {children}
      </BadgeAppContext.Provider>
    </>
  );
};

BadgeApp.propTypes = {
  app: PropTypes.object,
};

export const Attack = ({ dev, children, onClick }) => (
  <Card
    sx={{ maxWidth: 275 }}
    variant='outlined'
  >
    <CardActionArea onClick={onClick}>
      <CardContent>
        <Typography variant="h5" component="div">
          {dev.name}
        </Typography>
        <Typography
          sx={{ mb: 2 }}
          variant='body2'
          color={dev.color || 'primary'}
        >
          {dev.class}
        </Typography>
            
        <hr/>
        <Typography variant='body2'>
          {children}
        </Typography>
      </CardContent>
    </CardActionArea>
  </Card>
);

Attack.propTypes = {
  dev: PropTypes.object,
  children: PropTypes.node,
  onClick: PropTypes.func,
};

export const Device = ({ dev, onClick }) => (
  <Card
    sx={{ maxWidth: 300 }}
    variant='outlined'
  >
    <CardActionArea onClick={onClick}>
      <CardContent>
        {dev.stations && (
          <Typography sx={{ fontSize: 14 }} color="text.secondary" gutterBottom>
            {dev.stations.length} station{dev.stations.length > 1 && 's'}
          </Typography>
        )}
        <Typography variant="h5" component="div" sx={{ overflowWrap: "break-word" }}>
          {dev.mac}
        </Typography>
        <Typography sx={{ mb: 1.5 }} color="text.secondary">
          {dev.name || dev.ssid || ""}
        </Typography>
        <hr/>
        <Typography sx={{ mb: 1.5}} variant='body2'>
          {dev.channel && (<>CH: {dev.channel}<br/></>)}
          RSSI: {dev.rssi}
        </Typography>
        {dev.stations && (
          dev.stations.map((s) => (
            <Typography key={s.mac} sx={{ overflowWrap: "break-word" }}>
              STA: {s.mac}
            </Typography>
          ))
        )}
      </CardContent>
    </CardActionArea>
  </Card>
);

Device.propTypes = {
  dev: PropTypes.object,
  onClick: PropTypes.func,
};

export const Menu = ({
  apps,
  name,
  desc,
  onClick
}) => (
  <>
    <Typography variant='h3'>{name}</Typography>
    <Typography variant='body1' sx={{ mb: 5 }}>
      {desc}
    </Typography>
    <Grid container spacing={3}>
      {apps.map((a) => (
        <Grid item key={a.name} xs={12} sm={6} md={4} lg={3}>
          <Attack
            dev={a}
            onClick={() => onClick(a)}
          >
            {a.desc}
          </Attack>
        </Grid>
      ))}
    </Grid>
  </>
);

Menu.propTypes = {
  apps: PropTypes.arrayOf(PropTypes.object),
  name: PropTypes.string,
  desc: PropTypes.string,
  onClick: PropTypes.func,
};

export const sortDevs = (a, b) => {
  // Check if 'stations' exists and get its length, otherwise default to 0
  const aSL = a.stations ? a.stations.length : 0;
  const bSL = b.stations ? b.stations.length : 0;

  // First, sort by the length of the 'stations' array if they are different
  if (bSL !== aSL) {
    return bSL - aSL;
  }

  // If the lengths are the same, sort by 'rssi'
  return b.rssi - a.rssi;
};
