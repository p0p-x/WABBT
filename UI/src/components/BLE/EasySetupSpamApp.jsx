import { useNavigate } from 'react-router-dom';

import { BadgeApp } from '../common';

const app = {
  id: 'esspam',
  opt: 10,
  name: 'EasySetup Spam',
  class: 'offensive',
  color: 'error',
  desc: `
  Sends Samsung EasySetup
  advertisements that can
  cause popups on some phones.
`
};

export const EasySetupSpamApp = () => {
  const navigate = useNavigate();

  return (
    <BadgeApp
      app={app}
      onBack={() => navigate('/ble')}
    />
  );
};

export default EasySetupSpamApp;
