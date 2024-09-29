import { useNavigate } from "react-router-dom";

import { Menu } from './common';

const wifiAttacks = [{
  id: 'deauth',
  name: 'Deauth',
  class: 'offensive',
  color: 'error',
  desc: `
  Deauthenticates WIFI
  clients connected
  to networks. Can
  either target single
  stations or whole AP.
`}, {
  id: 'ddetect',
  name: 'Deauth Detect',
  class: 'defensive',
  color: 'primary',
  desc: `
  Detects deauthentiction
  frames that are sent. Useful
  to figure out if your
  network is under attack
  or other maliciousness.
`}, {
  id: 'pmkid',
  opt: 11,
  class: 'offensive',
  color: 'error',
  name: 'PMKID',
  desc: `
  Collects PMKIDs to
  later be cracked
  on a more powerful
  device.
`}];

export const WiFi = () => {
  const navigate = useNavigate();

  const onCardClick = (i) => {
    navigate(`/wifi/${i.id}`);
  };
  
  return (
    <Menu
      apps={wifiAttacks}
      name={'WiFi'}
      desc={'Tools for 2.4GHz WiFi Networks'}
      onClick={onCardClick}
    />
  );
};

export default WiFi;
