# BatteryEKF - general purpose battery class for embedded EKF
# Copyright (C) 2021 Dave Gutz
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation;
# version 2.1 of the License.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# Lesser General Public License for more details.
#
# See http://www.fsf.org/licensing/licenses/lgpl.txt for full license text.

"""1x1 General Purpose Extended Kalman Filter.   Inherit from this class and include ekf_predict and
ekf_update methods in the parent."""

global mon_old


class EKF1x1:
    """1x1 General Purpose Extended Kalman Filter.   Inherit from this class and include ekf_predict and
    ekf_update methods in the parent."""

    def __init__(self):
        """
        """
        self.Fx = 0.  # State transition
        self.Bu = 0.  # Control transition
        self.Q = 0.  # Process uncertainty
        self.R = 0.  # State uncertainty
        self.P = 0.  # Uncertainty covariance
        self.H = 0.  # Jacobian of h(x)
        self.S = 0.  # System uncertainty
        self.K = 0.  # Kalman gain
        self.hx = 0.  # Output of observation function h(x)
        self.u_ekf = 0.  # Control input
        self.x_ekf = 0.  # Kalman state variable
        self.y_ekf = 0.  # Residual z-hx
        self.z_ekf = 0.  # Observation of state x
        self.x_prior = self.x_ekf
        self.P_prior = self.P
        self.x_post = self.x_ekf
        self.P_post = self.P

    def __str__(self, prefix=''):
        """Returns representation of the object"""
        s = prefix + "EKF1x1:\n"
        s += "  Inputs:\n"
        s += "  z = {:10.6g}\n".format(self.z_ekf)
        s += "  R = {:10.6g}\n".format(self.R)
        s += "  Q = {:10.6g}\n".format(self.Q)
        s += "  H = {:10.6g}\n".format(self.H)
        s += "  Outputs:\n"
        s += "  x  = {:10.6g}\n".format(self.x_ekf)
        s += "  hx = {:10.6g}\n".format(self.hx)
        s += "  y  = {:10.6g}\n".format(self.y_ekf)
        s += "  P  = {:10.6g}\n".format(self.P)
        s += "  K  = {:10.6g}\n".format(self.K)
        s += "  S  = {:10.6g}\n".format(self.S)
        return s

    def ekf_predict(self):
        raise NotImplementedError

    def ekf_update(self):
        raise NotImplementedError

    def init_ekf(self, soc, p_init):
        """Initialize on demand"""
        self.x_ekf = soc
        self.P = p_init

    def predict_ekf(self, u, u_old=None):
        """1x1 Extended Kalman Filter predict
        Inputs:
            u   1x1 input, =ib, A
            Bu  1x1 control transition, Ohms
            Fx  1x1 state transition, V/V
        Outputs:
            x   1x1 Kalman state variable = Vsoc (0-1 fraction)
            P   1x1 Kalman probability
        """
        if u_old:
            self.u_ekf = u_old
        else:
            self.u_ekf = u
        self.Fx, self.Bu = self.ekf_predict()
        self.x_ekf = self.Fx*self.x_ekf + self.Bu*self.u_ekf
        self.P = self.Fx * self.P * self.Fx + self.Q
        self.x_prior = self.x_ekf
        self.P_prior = self.P

    def update_ekf(self, z, x_min, x_max, z_old=None):
        """ 1x1 Extended Kalman Filter update
            Inputs:
                z   1x1 input, =voc, dynamic predicted by other model, V
                R   1x1 Kalman state uncertainty
                Q   1x1 Kalman process uncertainty
                H   1x1 Jacobian sensitivity dV/dSOC
            Outputs:
                x   1x1 Kalman state variable = Vsoc (0-1 fraction)
                y   1x1 Residual z-hx, V
                P   1x1 Kalman uncertainty covariance
                K   1x1 Kalman gain
                S   1x1 system uncertainty
                SI  1x1 system uncertainty inverse
        """
        self.hx, self.H = self.ekf_update()
        if z_old:
            self.z_ekf = z_old
        else:
            self.z_ekf = z
        pht = self.P*self.H
        self.S = self.H*pht + self.R
        if abs(self.S) > 1e-12:
            self.K = pht/self.S  # using last-good-value if S=0
        self.y_ekf = self.z_ekf - self.hx
        self.x_ekf = max(min(self.x_ekf + self.K*self.y_ekf, x_max), x_min)
        i_kh = 1 - self.K*self.H
        self.P = i_kh*self.P
        self.x_post = self.x_ekf
        self.P_post = self.P

    def h_jacobian(self, x_ekf):
        # implemented by child
        raise NotImplementedError

    def hx_calc(self):
        # implemented by child
        raise NotImplementedError
