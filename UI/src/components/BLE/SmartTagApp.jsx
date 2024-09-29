import { useNavigate } from 'react-router-dom';

import { BadgeApp } from '../common';

const app = {
  id: 'smarttag',
  opt: 14,
  name: 'Smart Tagger',
  class: 'defensive',
  color: 'primary',
  desc: `
  Detects smart tags
  such as AirTags and
  Tiles. It will attempt
  to connect and set
  off an audio alert
  on the device.`
};

export const SmartTagApp = () => {
  const navigate = useNavigate();

  return (  
    <BadgeApp
      app={app}
      onBack={() => navigate('/ble')}
    />
  );
};

export default SmartTagApp;
