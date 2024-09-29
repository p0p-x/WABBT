import { useNavigate } from 'react-router-dom';

import { BadgeApp } from '../common';

const app = {
  id: 'blespamd',
  opt: 8,
  name: 'Spam Detect',
  class: 'defensive',
  color: 'primary',
  desc: `
  Detects various BLE spam
  advertisement attacks that
  are known to cause disruptive
  actions and unwanted popups.
`
};

export const SpamDetectApp = () => {
  const navigate = useNavigate();

  return (  
    <BadgeApp
      app={app}
      onBack={() => navigate('/ble')}
    />
  );
};

export default SpamDetectApp;
