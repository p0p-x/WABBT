import { useNavigate } from 'react-router-dom';

import { BadgeApp } from '../common';

const app = {
  id: 'bleskimd',
  opt: 12,
  name: 'Skimmer Detect',
  class: 'defensive',
  color: 'primary',
  desc: `
  Detects potential
  Gas Station Pump
  skimmers by looking
  for specific advertisements.
`
};

export const SkimmerDetectApp = () => {
  const navigate = useNavigate();

  return (  
    <BadgeApp
      app={app}
      onBack={() => navigate('/ble')}
    />
  );
};

export default SkimmerDetectApp;
