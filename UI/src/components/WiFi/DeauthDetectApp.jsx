import {
  useNavigate,
} from "react-router-dom";

import { BadgeApp } from '../common';

const app = {
  id: 'ddetect',
  opt: 6,
  name: 'Deauth Detect',
  class: 'defensive',
  color: 'primary',
  desc: `
  Detects deauthentiction
  frames that are sent. Useful
  to figure out if your
  network is under attack
  or other maliciousness.
`};

export const DeauthDetectApp = () => {
  const navigate = useNavigate();

  const onBack = (state, setState) => {
    if (state > 0) {
      stopDetect();
      setState(-1);
    } else {
      navigate('/wifi');
    }
  };

  return <BadgeApp app={app} onBack={onBack} />;
};

export default DeauthDetectApp;
