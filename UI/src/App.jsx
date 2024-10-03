// React
import React, { useState, useEffect } from 'react'
import {
  Outlet,
  createBrowserRouter,
  RouterProvider,
  useNavigate,
} from "react-router-dom";

// MUI Components
import { styled } from '@mui/material/styles';
import Box from '@mui/material/Box';
import MuiDrawer from '@mui/material/Drawer';
import MuiAppBar from '@mui/material/AppBar';
import Toolbar from '@mui/material/Toolbar';
import List from '@mui/material/List';
import Typography from '@mui/material/Typography';
import Divider from '@mui/material/Divider';
import IconButton from '@mui/material/IconButton';
import MenuIcon from '@mui/icons-material/Menu';
import SettingsIcon from '@mui/icons-material/Settings';
import WifiIcon from '@mui/icons-material/Wifi';
import ChevronLeftIcon from '@mui/icons-material/ChevronLeft';
import ListItem from '@mui/material/ListItem';
import ListItemButton from '@mui/material/ListItemButton';
import ListItemIcon from '@mui/material/ListItemIcon';
import ListItemText from '@mui/material/ListItemText';
import BluetoothIcon from '@mui/icons-material/Bluetooth';
import HomeIcon from '@mui/icons-material/Home';
import InfoIcon from '@mui/icons-material/Info';
import Container from '@mui/material/Container';
import Grid from '@mui/material/Grid';
import Alert from '@mui/material/Alert';

// Custom Components
import Home from './components/Home';

const drawerWidth = 200;

const openedMixin = (theme) => ({
  width: drawerWidth,
  transition: theme.transitions.create('width', {
    easing: theme.transitions.easing.sharp,
    duration: theme.transitions.duration.enteringScreen,
  }),
  overflowX: 'hidden',
});

const closedMixin = (theme) => ({
  transition: theme.transitions.create('width', {
    easing: theme.transitions.easing.sharp,
    duration: theme.transitions.duration.leavingScreen,
  }),
  overflowX: 'hidden',
  width: `calc(${theme.spacing(7)} + 1px)`,
  [theme.breakpoints.up('sm')]: {
    width: `calc(${theme.spacing(8)} + 1px)`,
  },
});

const DrawerHeader = styled('div')(({ theme }) => ({
  display: 'flex',
  alignItems: 'center',
  justifyContent: 'flex-end',
  padding: theme.spacing(0, 1),
  // necessary for content to be below app bar
  ...theme.mixins.toolbar,
}));

const AppBar = styled(MuiAppBar, {
  shouldForwardProp: (prop) => prop !== 'open',
})(({ theme, open }) => ({
  zIndex: theme.zIndex.drawer + 1,
  transition: theme.transitions.create(['width', 'margin'], {
    easing: theme.transitions.easing.sharp,
    duration: theme.transitions.duration.leavingScreen,
  }),
  ...(open && {
    marginLeft: drawerWidth,
    width: `calc(100% - ${drawerWidth}px)`,
    transition: theme.transitions.create(['width', 'margin'], {
      easing: theme.transitions.easing.sharp,
      duration: theme.transitions.duration.enteringScreen,
    }),
  }),
}));

const Drawer = styled(MuiDrawer, { shouldForwardProp: (prop) => prop !== 'open' })(
  ({ theme, open }) => ({
    width: drawerWidth,
    flexShrink: 0,
    whiteSpace: 'nowrap',
    boxSizing: 'border-box',
    ...(open && {
      ...openedMixin(theme),
      '& .MuiDrawer-paper': openedMixin(theme),
    }),
    ...(!open && {
      ...closedMixin(theme),
      '& .MuiDrawer-paper': closedMixin(theme),
    }),
  }),
);

export const Layout = () => {
  const navigate = useNavigate();
  const [open, setOpen] = useState(false);
  const [msg, setMsg] = useState('');
  const [alert, setAlert] = useState('success');
  const [showAlert, setShowAlert] = useState(false)

  const closeAlert = () => {
    setShowAlert(false);
    setMsg('');
  };

  const setupSocket = () => {
    if (!window.socket || window.socket.readyState === 3) {
      const socket = new WebSocket(`ws://1.3.3.7/ws`);

      socket.onopen = () => {
        console.log('WebSocket connection established');
        setShowAlert(false);
        setMsg('');

        if (window.socketCallback) {
          window.socketCallback({ connected: true });
        }
      };

      socket.onmessage = (message) => { 
        console.log('Message from server ', JSON.parse(message.data));
        const res = JSON.parse(message.data);

        if (res.hasOwnProperty('success')) {
          if (res.success == false) {
            setMsg(res.error);
            setAlert('error');
            setShowAlert(true);
          } else {
            setMsg('Success');
            setAlert('success');
            setShowAlert(true);

            if (window.onOptSuccess) {
              window.onOptSuccess();
            }
          }
        }

        if (window.socketCallback) {
          window.socketCallback(res);
        }
      };

      socket.onerror = (error) => {
        console.error('WebSocket error: ', error);
      };

      socket.onclose = () => {
        console.log('WebSocket connection closed');
        setMsg('No Connection');
        setAlert('error');
        setShowAlert(true);

        setTimeout(() => {
          setupSocket();
        }, 500);
      };

      window.socket = socket;  
    }
  };

  useEffect(() => {
    setupSocket();

    return () => {
      window.socket.close();
    };
  }, []);

  return (
    <Box sx={{ display: 'flex' }}>
      <AppBar position='fixed' open={open}>
        <Toolbar>
          <IconButton
            color='inherit'
            aria-label='open drawer'
            onClick={() => setOpen(true)}
            edge='start'
            sx={{
              marginRight: 5,
              ...(open && { display: 'none' }),
            }}
          >
            <MenuIcon />
          </IconButton>
          <Typography variant='h6' noWrap component='div'>
            WABBT
          </Typography>
        </Toolbar>
      </AppBar>
      <Drawer variant='permanent' open={open}>
        <DrawerHeader>
          <IconButton onClick={() => setOpen(false)}>
            <ChevronLeftIcon />
          </IconButton>
        </DrawerHeader>
        <Divider />
        <List>
          {['Home', 'WiFi', 'BLE', 'Settings', 'About'].map((text, i) => (
            <React.Fragment key={text}>
              { i === 3 && <Divider />}
              <ListItem key={text} disablePadding sx={{ display: 'block' }}>
                <ListItemButton
                  selected={window.location.pathname.split('/')[1] === `${text.toLowerCase()}`}
                  onClick={() => navigate(`/${text.toLowerCase()}`)}
                  sx={{
                    minHeight: 48,
                    justifyContent: open ? 'initial' : 'center',
                    px: 2.5,
                  }}
                >
                  <ListItemIcon
                    sx={{
                      minWidth: 0,
                      mr: open ? 3 : 'auto',
                      justifyContent: 'center',
                    }}
                  >
                    {i === 0 && <HomeIcon />}
                    {i === 1 && <WifiIcon />}
                    {i === 2 && <BluetoothIcon />}
                    {i === 3 && <SettingsIcon />}
                    {i === 4 && <InfoIcon />}
                  </ListItemIcon>
                  <ListItemText primary={text} sx={{ opacity: open ? 1 : 0 }} />
                </ListItemButton>
              </ListItem>
            </React.Fragment>
          ))}
        </List>
      </Drawer>
      <Box component='main' sx={{ flexGrow: 1, p: 3 }}>
        <DrawerHeader />
        <Container disableGutters maxWidth={false}>
          <Grid container>
            <Grid item xs={12}>
              {showAlert && (
                <Alert
                  severity={alert}
                  onClose={closeAlert}
                >
                  {msg}
                </Alert>
              )}

              <Outlet />
            </Grid>
          </Grid>
        </Container>
      </Box>
    </Box>
  );
};

const router = createBrowserRouter([
  {
    path: "/",
    element: <Layout />,
    children: [
      {
        index: true,
        element: <Home />,
      },
      {
        path: "home",
        element: <Home />
      },
      {
        path: "wifi",
        async lazy() {
          let { WiFi } = await import("./components/WiFi");
          return { Component: WiFi };
        },
      },
      {
        path: "wifi/deauth",
        async lazy() {
          let { DeauthApp } = await import("./components/WiFi/DeauthApp");
          return { Component: DeauthApp };
        }
      },
      {
        path: "wifi/ddetect",
        async lazy() {
          let { DeauthDetectApp } = await import("./components/WiFi/DeauthDetectApp");
          return { Component: DeauthDetectApp };
        }
      },
      {
        path: "/wifi/pmkid",
        async lazy() {
          let { PMKIDApp } = await import("./components/WiFi/PMKIDApp");
          return { Component: PMKIDApp };
        }
      },
      {
        path: "ble",
        async lazy() {
          let { Ble } = await import("./components/BLE");
          return { Component: Ble };
        },
      },
      {
        path: "ble/nspam",
        async lazy() {
          let { NameSpamApp } = await import("./components/BLE/NameSpamApp");
          return { Component: NameSpamApp };
        },
      },
      {
        path: "ble/gatk",
        async lazy() {
          let { GattAttackApp } = await import("./components/BLE/GattAttackApp");
          return { Component: GattAttackApp };
        },
      },
      {
        path: "ble/fpspam",
        async lazy() {
          let { FastPairSpamApp } = await import("./components/BLE/FastPairSpamApp");
          return { Component: FastPairSpamApp };
        },
      },
      {
        path: "ble/blespamd",
        async lazy() {
          let { SpamDetectApp } = await import("./components/BLE/SpamDetectApp");
          return { Component: SpamDetectApp };
        },
      },
      {
        path: "ble/vhci",
        async lazy() {
          let { VHCIApp } = await import("./components/BLE/VHCIApp");
          return { Component: VHCIApp };
        },
      },
      {
        path: "ble/esspam",
        async lazy() {
          let { EasySetupSpamApp } = await import("./components/BLE/EasySetupSpamApp");
          return { Component: EasySetupSpamApp };
        },
      },
      {
        path: "ble/bleskimd",
        async lazy() {
          let { SkimmerDetectApp } = await import("./components/BLE/SkimmerDetectApp");
          return { Component: SkimmerDetectApp };
        },
      },
      {
        path: "/ble/drone",
        async lazy() {
          let { DroneApp } = await import("./components/BLE/DroneApp");
          return { Component: DroneApp };
        }
      },
      {
        path: "/ble/smarttag",
        async lazy() {
          let { SmartTagApp } = await import("./components/BLE/SmartTagApp");
          return { Component: SmartTagApp };
        }
      },
      {
        path: "settings",
        async lazy() {
          let { Settings } = await import("./components/Settings");
          return { Component: Settings };
        },
      },
      {
        path: "about",
        async lazy() {
          let { About } = await import("./components/About");
          return { Component: About };
        },
      },
    ],
  },
]);

export function App() {
  return <RouterProvider router={router} fallbackElement={<p>Loading...</p>} />;
}

export default App;
