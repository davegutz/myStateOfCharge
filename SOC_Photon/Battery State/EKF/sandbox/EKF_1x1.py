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

"""1x1 General Purpose Extended Kalman Filter.   Inherit from this class and include ekf_model_predict and
ekf_model_update methods in the parent."""


class EKF_1x1:
    """1x1 General Purpose Extended Kalman Filter.   Inherit from this class and include ekf_model_predict and
    ekf_model_update methods in the parent."""

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
        self.u_kf = 0.  # Control input
        self.x_kf = 0.  # Kalman state variable
        self.y_kf = 0.  # Residual z-hx
        self.z_ekf = 0.  # Observation of state x
        self.x_prior = self.x_kf
        self.P_prior = self.P
        self.x_post = self.x_kf
        self.P_post = self.P

    def init_ekf(self, soc, p_init):
        """Initialize on demand"""
        self.x_kf = soc
        self.P = p_init

    def predict_ekf(self, u=None):
        """1x1 Extended Kalman Filter predict
        Inputs:
            u   1x1 input, =ib, A
            Bu  1x1 control transition, Ohms
            Fx  1x1 state transition, V/V
        Outputs:
            x   1x1 Kalman state variable = Vsoc (0-1 fraction)
            P   1x1 Kalman probability
        """
        self.u_kf = u
        self.x_kf = self.Fx*self.x_kf + self.Bu*self.u_kf
        self.P = self.Fx * self.P * self.Fx + self.Q
        self.x_prior = self.x_kf
        self.P_prior = self.P

    def update_ekf(self, z, h_jacobian_=None, hx_calc_=None):
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
        if h_jacobian_ is None:
            h_jacobian_ = self.h_jacobian
        if hx_calc_ is None:
            hx_calc_ = self.hx_calc
        self.z_ekf = z
        self.H = h_jacobian_(self.x_kf)
        pht = self.P*self.H
        self.S = self.H*pht + self.R
        self.K = pht/self.S
        self.hx = hx_calc_()
        self.y_kf = self.z_ekf - self.hx
        self.x_kf = self.x_kf + self.K*self.y_kf
        i_kh = 1 - self.K*self.H
        self.P = i_kh*self.P
        self.x_post = self.x_kf
        self.P_post = self.P

    def h_jacobian(self, x_kf):
        # implemented by child
        raise NotImplementedError

    def hx_calc(self):
        # implemented by child
        raise NotImplementedError
