# TrainTensor:  Use long data swing to train networks
# Copyright (C) 2023 Dave Gutz
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

""" Use Keras tensor flow to make models of dv = voc_stat - voc_soc"""

import math
import numpy as np
import pandas as pd
import tensorflow as tf
from matplotlib import pyplot as plt
from keras.models import Sequential
from keras.models import load_model, Model
from sklearn.model_selection import train_test_split
from keras.layers import Dense, LSTM, Dropout, Input, concatenate
from keras.losses import huber
from sklearn.metrics import mean_squared_error, mean_absolute_error
from keras.callbacks import EarlyStopping, ModelCheckpoint, ReduceLROnPlateau
from keras.saving import register_keras_serializable
from keras.backend import mean as keras_mean

# The following lines adjust the granularity of reporting.
pd.options.display.max_rows = 10
pd.options.display.float_format = "{:.1f}".format


# The Huber loss function wrapper so can input delta as kwarg
@register_keras_serializable()
def get_huber_loss_fn(**huber_loss_kwargs):

    @register_keras_serializable()
    def custom_huber_loss(y_true, y_pred):
        return huber(y_true, y_pred, **huber_loss_kwargs)

    return custom_huber_loss


def huber_loss_mean(y_true, y_pred, delta_huber=1.0):
    return keras_mean(huber(y_true, y_pred, delta_huber))


@register_keras_serializable()
def mre_loss(y_true, y_pred):
    square_root_difference = tf.math.sqrt(abs(y_true - y_pred))
    return tf.reduce_mean(square_root_difference, axis=-1)


# Load data frame from file
def load_data_file(file, attributes=None):
    data = pd.read_csv(file, skipinitialspace=True)
    data['voc'] = data['vb'] - data['dv_dyn']
    data['dv_hys_old'] = data['voc'] - data['voc_stat']
    data['dv'] = data['voc'] - data['voc_soc_new']
    data_df = data[attributes]
    return data_df, data


def parse_inputs(x, y, batch_size_, subsample_):
    rows = int(len(y) / batch_size_ / subsample_)
    x = resizer(x, rows, batch_size_, sub_samp=subsample_, n_in=2)
    x_vec = [x[:, :, 0], x[:, :, 1]]
    y = resizer(y, rows, batch_size_, sub_samp=subsample_)
    return x, y, x_vec


# Plotting function
def plot_the_loss_curve(epochs_, mse_training, mse_validation, use_mae=True):
    """Plot a curve of loss vs. epoch."""
    plt.figure()
    plt.xlabel("Epoch")
    if use_mae:
        plt.ylabel("Mean Absolute Error")
    else:
        plt.ylabel("Mean Squared Error")
    plt.plot(epochs_, mse_training, label="Training Loss")
    # plt.plot(epochs_, mse_validation, label="Validation Loss")   # no validation in model

    # mse_training is a pandas Series, so convert it to a list first.
    merged_mse_lists = np.append(mse_training.to_numpy(), mse_validation)
    highest_loss = max(merged_mse_lists)
    lowest_loss = min(merged_mse_lists)
    top_of_y_axis = highest_loss * 1.03
    bottom_of_y_axis = lowest_loss * 0.97

    plt.ylim([bottom_of_y_axis, top_of_y_axis])
    plt.legend()


# Plot hysteresis
def plot_hys(trn_x, trn_y, trn_predict):
    plt.figure()
    plt.plot(trn_x[:, 5, 1], trn_y[:, 5], label='dv_scaled_train')
    plt.plot(trn_x[:, 5, 1], trn_predict[:, 5], label='dv_scaled_predict')
    plt.legend(loc=3)
    plt.xlabel('soc')
    plt.ylabel('dv_scaled')
    plt.title('Hysteresis for Training and Prediction')


# Plot fail detection predictions
def plot_fail(trn_y, trn_predict, trn_predict_fail_ib, t_samp_, ib_bias_):
    n_fail = trn_predict_fail_ib.shape[0]
    rows = len(trn_y)
    time = range(rows) * t_samp_
    plt.figure(figsize=(15, 6), dpi=80)
    plt.plot(time, trn_y[:, 5, :], label='actual')
    plt.plot(time, trn_predict[:, 5, :], label='prediction')
    for i in range(n_fail):
        label_ = "prediction with {:5.2f} A".format(ib_bias_[i])
        plt.plot(time, trn_predict_fail_ib[i, :, 5, :], label=label_)
    plt.legend(loc=3)
    plt.xlabel('seconds')
    plt.ylabel('dv scaled')
    plt.title('Failure Effects at Steady State')


# Plot the inputs
def plot_input(trn_x, trn_y, val_x, val_y, tst_x, tst_y, t_samp_, use_ib_lag_):
    inp_ib = np.append(trn_x[:, :, 0], val_x[:, :, 0])
    inp_ib = np.append(inp_ib, tst_x[:, :, 0])
    inp_soc = np.append(trn_x[:, :, 1], val_x[:, :, 1])
    inp_soc = np.append(inp_soc, tst_x[:, :, 1])
    inp_dv = np.append(trn_y[:, :, 0], val_y[:, :, 0])
    inp_dv = np.append(inp_dv, tst_y[:, :, 0])
    rows = len(inp_ib)  # inp is unstacked while all the x's are stacked
    plt.figure(figsize=(15, 6), dpi=80)
    time = range(rows) * t_samp_
    plt.plot(time, inp_dv, label='dv_scaled', color='blue')
    if use_ib_lag_:
        plt.plot(time, inp_ib,  label='ib_lag scaled', color='orange')
    else:
        plt.plot(time, inp_ib, label='ib scaled', color='orange')
    plt.plot(time, inp_soc, label='soc', color='black')
    trn_len = trn_x.shape[0]*trn_x.shape[1]  # x's are stacked
    val_len = val_x.shape[0]*val_x.shape[1]  # x's are stacked
    plt.axvline(x=trn_len*t_samp_, color='r')
    plt.axvline(x=(trn_len+val_len)*t_samp_, color='m')
    plt.legend(loc=3)
    plt.xlabel('Observation number after given time steps')
    plt.xlabel('seconds')
    if use_ib_lag_:
        plt.ylabel('ib_lag scaled / soc / dv scaled')
    else:
        plt.ylabel('ib scaled / soc / dv scaled')
    plt.title('Inputs.  The Red Line Separates The Training, Validation, and Test Examples')


# Plot the results
def plot_result(trn_y, val_y, tst_y, trn_pred, val_pred, tst_pred, trn_dv_hys_old, val_dv_hys_old, tst_dv_hys_old, t_samp_,
                scale_in_dv_=1.):
    actual_train = np.append(trn_y, val_y)
    actual = np.append(actual_train, tst_y)
    predictions_train = np.append(trn_pred, val_pred)
    predictions = np.append(predictions_train, tst_pred)
    errors_train = -(predictions_train - actual_train)*scale_in_dv_*1000.
    errors = -(predictions - actual)*scale_in_dv_*1000.
    dv_hys_old_train = np.append(trn_dv_hys_old, val_dv_hys_old)
    dv_hys_old = np.append(dv_hys_old_train, tst_dv_hys_old)
    errors_old_train = -(dv_hys_old_train - actual_train)*scale_in_dv_*1000.
    errors_old = -(dv_hys_old - actual)*scale_in_dv_*1000.
    rows = len(actual)
    time = range(rows) * t_samp_
    rows_train_ = len(actual_train)
    time_train = range(rows_train_) * t_samp_
    plt.figure(figsize=(15, 12), dpi=80)
    plt.subplot(2, 1, 1)
    plt.title('Actual and Predicted Values and old model. The Red Line Separates The Training, Validation, and Test Examples')
    plt.plot(time, actual, color='black')
    plt.plot(time, predictions, color='orange')
    plt.plot(time, dv_hys_old, color='green')
    plt.axvline(x=len(trn_y)*t_samp_, color='r')
    plt.axvline(x=(len(trn_y)+len(val_y))*t_samp_, color='m')
    plt.legend(['Actual', 'Predictions', 'dv_hys'])
    plt.xlabel('seconds')
    plt.ylabel('dv scaled')
    plt.grid()
    plt.subplot(2, 1, 2)
    plt.plot(time_train, actual_train, color='black')
    plt.plot(time_train, predictions_train, color='orange')
    plt.plot(time_train, dv_hys_old_train, color='green')
    plt.axvline(x=len(trn_y)*t_samp_, color='r')
    plt.axvline(x=(len(trn_y)+len(val_y))*t_samp_, color='m')
    plt.legend(['Actual', 'Predictions', 'dv_hys_old'])
    plt.xlabel('seconds')
    plt.ylabel('dv scaled')
    plt.grid()

    plt.figure(figsize=(15, 12), dpi=80)
    plt.subplot(2, 1, 1)
    plt.title('Error Values. The Red Line Separates The Training, Validation, and Test Examples')
    plt.plot(time, errors, color='orange')
    plt.plot(time, errors_old, color='green')
    plt.axvline(x=len(trn_y)*t_samp_, color='r')
    plt.axvline(x=(len(trn_y)+len(val_y))*t_samp_, color='m')
    plt.legend(['Error, mV', 'Error_old, mV'])
    plt.xlabel('seconds')
    plt.ylabel('dv, mV')
    plt.grid()
    plt.subplot(2, 1, 2)
    plt.plot(time_train, errors_train, color='orange')
    plt.plot(time_train, errors_old_train, color='green')
    plt.axvline(x=len(trn_y)*t_samp_, color='r')
    plt.axvline(x=(len(trn_y)+len(val_y))*t_samp_, color='m')
    plt.legend(['Error, mV', 'Error_old, mV'])
    plt.xlabel('seconds')
    plt.ylabel('dv, mV')
    plt.grid()


def print_error(trn_y, val_y, tst_y, trn_pred, val_pred, tst_pred, delta_=1.):
    # Error of predictions
    trn_rmse = math.sqrt(mean_squared_error(trn_y, trn_pred))
    val_rmse = math.sqrt(mean_squared_error(val_y, val_pred))
    tst_rmse = math.sqrt(mean_squared_error(tst_y, tst_pred))

    trn_mae = mean_absolute_error(trn_y, trn_pred)
    val_mae = mean_absolute_error(val_y, val_pred)
    tst_mae = mean_absolute_error(tst_y, tst_pred)

    trn_mre = np.mean(np.sqrt(np.abs(trn_y - trn_pred)))
    val_mre = np.mean(np.sqrt(np.abs(val_y - val_pred)))
    tst_mre = np.mean(np.sqrt(np.abs(tst_y - tst_pred)))

    trn_hub = huber_loss_mean(trn_y, trn_pred, delta_huber=delta_)
    val_hub = huber_loss_mean(val_y, val_pred, delta_huber=delta_)
    tst_hub = huber_loss_mean(tst_y, tst_pred, delta_huber=delta_)

    # Print
    print("Train    RMSE {:6.3f}".format(trn_rmse), " ||  MAE {:6.3f}".format(trn_mae), " ||  MRE {:6.3f}".format(trn_mre), " || HUB {:6.3f}".format(trn_hub))
    print("Validate RMSE {:6.3f}".format(val_rmse), " ||  MAE {:6.3f}".format(val_mae), " ||  MRE {:6.3f}".format(val_mre), " || HUB {:6.3f}".format(val_hub))
    print("Test     RMSE {:6.3f}".format(tst_rmse), " ||  MAE {:6.3f}".format(tst_mae), " ||  MRE {:6.3f}".format(tst_mre), " || HUB {:6.3f}".format(tst_hub))


def process_battery_attributes(df, scale=(50., 10., 1., 0.1), limit_dv=False):
    # column names of continuous features
    data = df[['Tb', 'ib', 'soc', 'ib_lag', 'dv_hys_old']]
    s_Tb, s_ib, s_soc, s_dv = scale
    with pd.option_context('mode.chained_assignment', None):
        data['Tb'] = data['Tb'].map(lambda x: x/s_Tb)
        data['ib'] = data['ib'].map(lambda x: x / s_ib)
        data['ib_lag'] = data['ib_lag'].map(lambda x: x/s_ib)
        data['soc'] = data['soc'].map(lambda x: x/s_soc)
        data['dv_hys_old'] = data['dv_hys_old'].map(lambda x: x/s_dv)
        y = df['dv'] / s_dv
        if limit_dv:
            y = y.map(lambda x: max(min(x, s_dv), -s_dv))
    return data, y


# Resize and convert to numpy
def resizer(v, targ_rows, btch_size, sub_samp=1, n_in=1):
    v = v.to_numpy('float')[:targ_rows*btch_size*sub_samp]
    rows_raw = len(v)
    x = v.reshape(int(rows_raw/sub_samp), sub_samp, n_in)[:, 0, :]
    return x.reshape(targ_rows, btch_size, n_in)


# Single input with one-hot augmentation path
def tensor_model_create(hidden_units, input_shape, dense_units=1, activation=None, learn_rate=0.01, drop=0.2,
                        use_two_lstm=False, use_mae=True, use_mre_loss_=False,
                        use_huber_loss_=False, huber_delta_=1.):
    if activation is None:
        activation = ['relu', 'sigmoid', 'linear', 'linear']
    simple_input = (input_shape[0], 1)
    ib_in = Input(shape=simple_input, name='ib-in')
    soc_in = Input(shape=simple_input, name='soc-in')

    # LSTM using ib
    lstm = Sequential()
    lstm.add(ib_in)
    lstm.add(LSTM(hidden_units, input_shape=simple_input, activation=activation[0],
                  recurrent_activation=activation[1], return_sequences=True))
    lstm.add(Dropout(drop))
    if use_two_lstm:
        lstm.add(LSTM(hidden_units, activation=activation[0],
                      recurrent_activation=activation[1], return_sequences=True))
        lstm.add(Dropout(drop))
    lstm.add(Dense(units=1, activation=activation[2]))
    lstm.summary()

    # Implement the final configuration
    merged = concatenate([lstm.output, soc_in])
    predictions = Dense(units=dense_units, activation=activation[2])(merged)
    final = Model(inputs=[ib_in, soc_in], outputs=predictions)
    if use_mae:
        loss_ = 'mean_absolute_error'
    else:
        loss_ = 'mean_squared_error'
    if use_mre_loss_:
        loss_ = mre_loss
    if use_huber_loss_:
        loss_ = get_huber_loss_fn(delta=huber_delta_)
    final.compile(loss=loss_, optimizer=tf.keras.optimizers.Adam(learning_rate=learn_rate),
                  metrics=['mse', 'mae'])
    final.summary()

    return final


# Single input with one-hot augmentation path
def tensor_model_create_many_to_one(hidden_units, input_shape, activation=None, learn_rate=0.01,
                                    drop=0.2, use_two_lstm=False, use_mae=True, use_mre_loss_=False,
                                    use_huber_loss_=False, huber_delta_=1.):
    if activation is None:
        activation = ['relu', 'sigmoid', 'linear', 'linear']
    dual_input = (input_shape[0], 2)
    both_in = Input(shape=dual_input, name='ib-soc-in')

    # LSTM using all_in
    lstm = Sequential()
    lstm.add(both_in)
    lstm.add(LSTM(hidden_units, input_shape=dual_input, activation=activation[0],
                  recurrent_activation=activation[1], return_sequences=True))
    lstm.add(Dropout(drop))
    if use_two_lstm:
        lstm.add(LSTM(hidden_units, activation=activation[0],
                      recurrent_activation=activation[1], return_sequences=True))
        lstm.add(Dropout(drop))
    lstm.add(Dense(units=1, activation=activation[2]))
    if use_mae:
        loss_ = 'mean_absolute_error'
    else:
        loss_ = 'mean_squared_error'
    if use_mre_loss_:
        loss_ = mre_loss
    if use_huber_loss_:
        loss_ = get_huber_loss_fn(delta=huber_delta_)
    lstm.compile(loss=loss_, optimizer=tf.keras.optimizers.Adam(learning_rate=learn_rate),
                 metrics=['mse', 'mae'])
    lstm.summary()

    return lstm


def train_model(mod, x, y, epochs_=20, btch_size=1, verbose=0, patient=10, use_many_=False):
    """Feed a dataset into the model in order to train it."""

    # Training callbacks
    es = EarlyStopping(monitor='loss', mode='min', verbose=1, patience=patient)
    if use_many_:
        file_path = 'TrainTensor_many_lstm.h5'
    else:
        file_path = 'TrainTensor_lstm.h5'
    mc = ModelCheckpoint(file_path, monitor='loss', mode='min', verbose=1, save_weights_only=False, save_best_only=True)
    rp = ReduceLROnPlateau(monitor='loss', factor=0.5, patience=int(patient/2), verbose=1, mode='auto', min_delta=0.0001,
                      cooldown=5, min_lr=0)
    cb = [es, mc, rp]
    # cb = es

    # train the model
    hist = mod.fit(x=x, y=y, epochs=epochs_, batch_size=btch_size, shuffle=False, verbose=verbose, callbacks=cb)

    # Get details that will be useful for plotting the loss curve.
    epochs_ = hist.epoch
    ret_hist = pd.DataFrame(hist.history)
    mserr = ret_hist["loss"]
    return epochs_, mserr, ret_hist


def train_tensor_lstm():
    # Inputs shouldn't have to change
    val_fraction = 0.25
    dropping = 0.2

    # Inputs frequently changed
    train_file = ".//temp//dv_20230831_soc0p_ch_clip_clean.csv"
    # train_file = ".//temp//dv_train_soc0p_ch_clip_clean.csv"
    validate_file = ".//temp//dv_validate_soc0p_ch_clip_clean.csv"
    test_file = ".//temp//dv_test_soc0p_ch_clean.csv"
    params = ['Tb', 'ib', 'soc', 'sat', 'ib_lag', 'dv_hys_old', 'dv']
    fit_mae = True
    use_many = False
    scale_in = (50., 10., 1., 0.25)
    # scale_in = (1., 1., 1., 1.)
    scale_in_dv = scale_in[3]
    use_two = False
    use_ib_lag = True  # 180 sec in Chemistry_BMS.py IB_LAG_CH
    learning_rate = 0.0003
    epochs_lim = 750
    # epochs_lim = 5
    hidden = 4
    subsample = 5
    nom_batch_size = 30
    patience = 40
    ib_bias = [.1, .5, 1.]
    try_load_model = False
    use_mre_loss = True
    use_huber_loss = True
    huber_delta = 0.1
    use_dv_input_limit = False

    # Adjust model automatically
    if use_two:
        learning_rate *= 2
    if use_many:
        save_path = 'TrainTensor_many_lstm.keras'
        hidden *= 2
    else:
        save_path = 'TrainTensor_lstm.keras'
    batch_size = int(nom_batch_size / subsample)

    print("[INFO] loading train/validation attributes...")
    train_df, train = load_data_file(train_file, params)
    if validate_file is not None:
        validate_attr, validate = load_data_file(validate_file, params)
        train_attr = train_df
    else:
        train_attr, validate_attr = train_test_split(train_df, test_size=val_fraction, shuffle=False)

    print("[INFO] loading test attributes...")
    test_df, test = load_data_file(test_file, params)
    test_attr = test_df

    # Adjust data
    train_attr, train_y = process_battery_attributes(train_attr, scale=scale_in, limit_dv=use_dv_input_limit)
    validate_attr, validate_y = process_battery_attributes(validate_attr, scale=scale_in)
    test_attr, test_y = process_battery_attributes(test_attr, scale=scale_in)
    if use_ib_lag:
        train_x = train_attr[['ib_lag', 'soc']]
        validate_x = validate_attr[['ib_lag', 'soc']]
        test_x = test_attr[['ib_lag', 'soc']]
    else:
        train_x = train_attr[['ib', 'soc']]
        validate_x = validate_attr[['ib', 'soc']]
        test_x = test_attr[['ib', 'soc']]

    # Setup model
    rows_train = int(len(train_y) / batch_size / subsample)
    train_x, train_y, train_x_vec = parse_inputs(train_x, train_y, batch_size, subsample)

    rows_val = int(len(validate_y) / batch_size / subsample)
    validate_x, validate_y, validate_x_vec = parse_inputs(validate_x, validate_y, batch_size, subsample)

    rows_tst = int(len(test_y) / batch_size / subsample)
    test_x, test_y, test_x_vec = parse_inputs(test_x, test_y, batch_size, subsample)

    # Optional make new model
    if try_load_model:
        model = load_model(save_path)
    else:
        if use_many:
            model = tensor_model_create_many_to_one(hidden_units=hidden, input_shape=(train_x.shape[1], train_x.shape[2]),
                                                    activation=['relu', 'sigmoid', 'linear', 'linear'],
                                                    learn_rate=learning_rate, drop=dropping, use_two_lstm=use_two,
                                                    use_mae=fit_mae, use_mre_loss_=use_mre_loss,
                                                    use_huber_loss_=use_huber_loss, huber_delta_=huber_delta)
        else:
            model = tensor_model_create(hidden_units=hidden, input_shape=(train_x.shape[1], train_x.shape[2]),
                                        dense_units=1, activation=['relu', 'sigmoid', 'linear', 'linear'],
                                        learn_rate=learning_rate, drop=dropping, use_two_lstm=use_two, use_mae=fit_mae,
                                        use_mre_loss_=use_mre_loss, use_huber_loss_=use_huber_loss,
                                        huber_delta_=huber_delta)

    # Train model
    print("[INFO] training model...")
    if use_many:
        epochs, mse, history = train_model(model, train_x, train_y, epochs_=epochs_lim, btch_size=batch_size, verbose=1,
                                           patient=patience, use_many_=use_many)
    else:
        epochs, mse, history = train_model(model, train_x_vec, train_y, epochs_=epochs_lim, btch_size=batch_size, verbose=1,
                                           patient=patience, use_many_=use_many)
    model.save(save_path)
    plot_the_loss_curve(epochs, mse, history["loss"], use_mae=fit_mae)

    # make predictions
    print("[INFO] predicting 'dv'...")
    if use_many:
        train_predict = model.predict(train_x)
        validate_predict = model.predict(validate_x)
        test_predict = model.predict(test_x)
    else:
        train_predict = model.predict(train_x_vec)
        validate_predict = model.predict(validate_x_vec)
        test_predict = model.predict(test_x_vec)
    train_x_fail_ib = train_x.copy()
    train_predict_fail_ib = []
    for fail_ib_mag in ib_bias:
        train_x_fail_ib[:, :, 0] += fail_ib_mag / scale_in[1]
        if use_many:
            train_predict_fail_ib.append(model.predict(train_x_fail_ib))
        else:
            train_x_fail_ib_vec = [train_x_fail_ib[:, :, 0], train_x_fail_ib[:, :, 1]]
            train_predict_fail_ib.append(model.predict(train_x_fail_ib_vec))

    # Plot result
    t_samp = train['cTime'][20]-train['cTime'][19]
    t_samp_input = t_samp * subsample
    t_samp_result = t_samp_input * batch_size
    train_dv_hys_old = resizer(train_attr[['dv_hys_old']], rows_train, batch_size, sub_samp=subsample)
    validate_dv_hys_old = resizer(validate_attr[['dv_hys_old']], rows_val, batch_size, sub_samp=subsample)
    test_dv_hys_old = resizer(test_attr[['dv_hys_old']], rows_tst, batch_size, sub_samp=subsample)
    plot_input(trn_x=train_x, trn_y=train_y, val_x=validate_x, val_y=validate_y, tst_x=test_x, tst_y=test_y,
               t_samp_=t_samp_input, use_ib_lag_=use_ib_lag)
    plot_result(trn_y=train_y[:, batch_size-1, :], val_y=validate_y[:, batch_size-1, :], tst_y=test_y[:, batch_size-1, :],
                trn_pred=train_predict[:, batch_size-1, :], val_pred=validate_predict[:, batch_size-1, :],
                tst_pred=test_predict[:, batch_size-1, :],
                trn_dv_hys_old=train_dv_hys_old[:, batch_size-1, :], val_dv_hys_old=validate_dv_hys_old[:, batch_size-1, :],
                tst_dv_hys_old=test_dv_hys_old[:, batch_size-1, :],
                t_samp_=t_samp_result, scale_in_dv_=scale_in_dv)
    plot_hys(train_x, train_y, train_predict)
    plot_fail(train_y, train_predict, np.array(train_predict_fail_ib), t_samp_=t_samp_result, ib_bias_=ib_bias)

    # Print model
    model.summary()
    print('\nShape:')
    for wt in model.layers[1].weights:
        print(wt.name, '-->', wt.shape)
        print(wt.numpy())

    # look at h5 file
    print('\nBest:')
    classifier = load_model(save_path)
    print(classifier.summary())

    # Print error
    print_error(trn_y=train_y[:, batch_size-1, :], val_y=validate_y[:, batch_size-1, :], tst_y=test_y[:, batch_size-1, :],
                trn_pred=train_predict[:, batch_size-1, :], val_pred=validate_predict[:, batch_size-1, :], tst_pred=test_predict[:, batch_size-1, :],
                delta_=huber_delta)

    plt.show()


if __name__ == '__main__':
    train_tensor_lstm()

"""
        es = EarlyStopping(monitor='val_loss', mode='min', verbose=1, patience=50) --> Often, the first sign of no further improvement may not be the best time to stop training. This is because the model may coast into a plateau of no improvement or even get slightly worse before getting much better. We can account for this by adding a delay to the trigger in terms of the number of epochs on which we would like to see no improvement. This can be done by setting the “patience” argument.

        es = EarlyStopping(monitor='val_accuracy', mode='max', min_delta=1) --> By default, any change in the performance measure, no matter how fractional, will be considered an improvement. You may want to consider an improvement that is a specific increment, such as 1 unit for mean squared error or 1% for accuracy. This can be specified via the “min_delta” argument.

        es = EarlyStopping(monitor='val_loss', mode='min', baseline=0.4) --> Finally, it may be desirable to only stop training if performance stays above or below a given threshold or baseline. For example, if you have familiarity with the training of the model (e.g. learning curves) and know that once a validation loss of a given value is achieved that there is no point in continuing training. This can be specified by setting the “baseline” argument.

        mc = ModelCheckpoint('best_model.h5', monitor='val_loss', mode='min', verbose=1) --> The EarlyStopping callback will stop training once triggered, but the model at the end of training may not be the model with best performance on the validation dataset. An additional callback is required that will save the best model observed during training for later use. This is the ModelCheckpoint callback.
"""
# Tb_boundaries = np.linspace(0, 50, 3)
# inputs = {
#     'Tb':
#         tf.keras.layers.Input(shape=(1,), dtype=tf.float32, name='Tb'),
#     'ib':
#         tf.keras.layers.Input(shape=(1,), dtype=tf.float32, name='ib'),
#     'soc':
#         tf.keras.layers.Input(shape=(1,), dtype=tf.float32, name='soc'),
# }
# Tb = tf.keras.layers.Normalization(name='Tb_norm', axis=None)
# Tb.adapt(train_df['Tb'])
# Tb = Tb(inputs.get('Tb'))
# Tb = tf.keras.layers.Discretization(bin_boundaries=Tb_boundaries, name='discretization_Tb')(Tb)
