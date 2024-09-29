import { useState } from 'react'; 
import { useNavigate } from 'react-router-dom';

import TextField from '@mui/material/TextField';

import { BadgeApp, BadgeAppContext } from '../common';

const app = {
  id: 'nspam',
  opt: 2,
  name: 'Name Spam',
  desc: `
  Sends advertisments usings
  random MAC addresses. This
  will appear in your phones
  bluetooth when going to pair.`
};

export const NameSpamApp = () => {
  const navigate = useNavigate();
  const [name, setName] = useState('');

  const startNameSpamApp = (e) => {
    e.preventDefault();
    if (window.socket?.readyState === 1) {
      window.socket.send(JSON.stringify({
        opt: app.opt,
        action: "start",
        name,
      }));
    }
  };

  return (
    <BadgeApp
      app={app}
      startFunc={startNameSpamApp}
      onBack={() => navigate('/ble')}
    >
      <BadgeAppContext.Consumer>
        {({ state }) => (
          <>
            {state === 0 && (
              <form onSubmit={(e) => e.preventDefault()}>
                <TextField
                  value={name}
                  onChange={(e) => setName(e.target.value)}
                  size='small'
                  placeholder='Hack tha plan8'
                />
              </form>
            )}
          </>
        )}
      </BadgeAppContext.Consumer>
    </BadgeApp>
  );
};

export default NameSpamApp;
