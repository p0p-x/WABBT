import { useNavigate } from 'react-router-dom';

import { BadgeApp } from '../common';

const app = {
  id: 'fpspam',
  opt: 7,
  name: 'FastPair Spam',
  class: 'offensive',
  color: 'error',
  desc: `
  Sends Google FastPair BLE
  advertisements that can cause
  popups on some phones.
`
};

export const FastPairSpamApp = () => {
  const navigate = useNavigate();

  return (
    <BadgeApp
      app={app}
      onBack={() => navigate('/ble')}
    />
  );
};

export default FastPairSpamApp;
