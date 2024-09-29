import { useState } from 'react';
import { useNavigate } from 'react-router-dom';

import Button from '@mui/material/Button';
import TextField from '@mui/material/TextField';

import { BadgeApp, BadgeAppContext } from '../common';

const app = {
  id: 'vhci',
  opt: 9,
  name: 'vHCI',
  class: 'tool',
  color: 'primary',
  desc: `
  Virtual HCI for the
  Bluetooth controller.
`
};

export const VHCIApp = () => {
  const navigate = useNavigate();
  const [input, setInput] = useState('');

  const sendHCI = (e, setOutput) => {
    e.preventDefault();
    console.log('Send', input);
    if (window.socket?.readyState === 1) {
      window.socket.send(JSON.stringify({
        opt: app.opt,
        action: "send",
        input,
      }));
      setOutput((o) => `${o}\nSend: ${input}`);
    }
  };

  return (
    <BadgeApp
      app={app}
      onBack={() => navigate('/ble')}
    >
      <BadgeAppContext.Consumer>
        {({ state, setOutput }) => (
          <>
            {state === 1 && (
              <form onSubmit={(e) => sendHCI(e, setOutput)}>
                <TextField
                  value={input}
                  onChange={(e) => setInput(e.target.value)}
                  size='small'
                  placeholder='HCI Cmd:'
                />
                <Button
                  sx={{ ml: 2 }}
                  type='submit'
                  variant='contained'
                  color='primary'
                  onClick={(e) => sendHCI(e, setOutput)}
                >
                  Send
                </Button>
              </form>
            )}
          </>
        )}
      </BadgeAppContext.Consumer> 
    </BadgeApp>
  );
};

export default VHCIApp;
