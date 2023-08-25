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
from keras.layers import Dense, LSTM, Dropout, Input, concatenate
from sklearn.model_selection import train_test_split
from keras.callbacks import EarlyStopping
from keras.callbacks import ModelCheckpoint
from TrainTensorSatSoc_lstm import plot_result, plot_input, plot_the_loss_curve, plot_hys, plot_fail, print_error

# The following lines adjust the granularity of reporting.
pd.options.display.max_rows = 10
pd.options.display.float_format = "{:.1f}".format


def process_battery_attributes(df, scale=(50., 10., 1., 0.1)):
    # column names of continuous features
    data = df[['Tb', 'ib', 'soc', 'ib_lag', 'dv_hys']]
    s_Tb, s_ib, s_soc, s_dv = scale
    with pd.option_context('mode.chained_assignment', None):
        data['Tb'] = data['Tb'].map(lambda x: x/s_Tb)
        data['ib'] = data['ib'].map(lambda x: x / s_ib)
        data['ib_lag'] = data['ib_lag'].map(lambda x: x/s_ib)
        data['soc'] = data['soc'].map(lambda x: x/s_soc)
        data['dv_hys'] = data['dv_hys'].map(lambda x: x/s_dv)
        y = df['dv'] / s_dv

    return data, y


# Resize and convert to numpy
def resizer(v, targ_rows, btch_size, sub_samp=1, n_in=1):
    v = v.to_numpy('float')[:targ_rows*btch_size*sub_samp]
    rows_raw = len(v)
    x = v.reshape(int(rows_raw/sub_samp), sub_samp, n_in)[:, 0, :]
    return x.reshape(targ_rows, btch_size, n_in)


# Single input with one-hot augmentation path
def tensor_model_create(hidden_units, input_shape, dense_units=1, activation=None, learn_rate=0.01, drop=0.2,
                        use_two_lstm=False):
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
    final.compile(loss='mean_squared_error', optimizer=tf.keras.optimizers.Adam(learning_rate=learn_rate),
                  metrics=['mse', 'mae'])
    final.summary()

    return final


# Single input with one-hot augmentation path
def tensor_model_create_many_to_one(hidden_units, input_shape, dense_units=1, activation=None, learn_rate=0.01,
                                    drop=0.2, use_two_lstm=False, fit_mae=False):
    if activation is None:
        activation = ['relu', 'sigmoid', 'linear', 'linear']
    simple_input = (input_shape[0], 1)
    ib_in = Input(shape=simple_input, name='ib-in')
    soc_in = Input(shape=simple_input, name='soc-in')
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
    if fit_mae:
        loss_  = 'mean_absolute_error'
    else:
        loss_ = 'mean_squared_error'
    lstm.compile(loss=loss_, optimizer=tf.keras.optimizers.Adam(learning_rate=learn_rate),
                 metrics=['mse', 'mae'])
    lstm.summary()

    return lstm


def train_model(mod, x, y, epochs_=20, btch_size=1, verbose=0, patient=10):
    """Feed a dataset into the model in order to train it."""

    # Training callbacks
    es = EarlyStopping(monitor='loss', mode='min', verbose=1, patience=patient)
    # file_path = 'TrainTensorSatSoc_many_lstm.keras'
    # mc = ModelCheckpoint(file_path, monitor='loss', mode='min', verbose=1, save_weights_only=False, save_best_only=True)
    # cb = [es, mc]
    cb = [es]

    # train the model
    hist = mod.fit(x=x, y=y, epochs=epochs_, batch_size=btch_size, shuffle=False, verbose=verbose, callbacks=cb)

    # Get details that will be useful for plotting the loss curve.
    epochs_ = hist.epoch
    ret_hist = pd.DataFrame(hist.history)
    mserr = ret_hist["loss"]
    return epochs_, mserr, ret_hist


# Load data and normalize.  Using inputs from these _rep files so doesn't matter that they're derived
train_file = ".//temp//dv_train_soc0p_ch_clip_clean.csv"
validate_file = ".//temp//dv_validate_soc0p_ch_clip_clean.csv"
test_file = ".//temp//dv_test_soc0p_ch_clean.csv"
val_fraction = 0.25

print("[INFO] loading train/validation attributes...")
train = pd.read_csv(train_file, skipinitialspace=True)
train['voc'] = train['vb'] - train['dv_dyn']
train['dv_hys'] = train['voc'] - train['voc_stat']
train['dv'] = train['voc'] - train['voc_soc']
train_df = train[['Tb', 'ib', 'soc', 'sat', 'ib_lag', 'dv_hys', 'dv']]

if validate_file is not None:
    validate = pd.read_csv(validate_file, skipinitialspace=True)
    validate['voc'] = validate['vb'] - validate['dv_dyn']
    validate['dv_hys'] = validate['voc'] - validate['voc_stat']
    validate['dv'] = validate['voc'] - validate['voc_soc']
    validate_df = validate[['Tb', 'ib', 'soc', 'sat', 'ib_lag', 'dv_hys', 'dv']]
else:
    validate_df = None

print("[INFO] loading test attributes...")
test = pd.read_csv(test_file, skipinitialspace=True)
test['voc'] = test['vb'] - test['dv_dyn']
test['dv_hys'] = test['voc'] - test['voc_stat']
test['dv'] = test['voc'] - test['voc_soc']
test_df = test[['Tb', 'ib', 'soc', 'sat', 'ib_lag', 'dv_hys', 'dv']]

# Split training data into train and validation data frames
if validate_file is None:
    train_attr, validate_attr = train_test_split(train_df, test_size=val_fraction, shuffle=False)
else:
    train_attr = train_df
    validate_attr = validate_df
test_attr = test_df

# Create model
fit_mae = True
scale_in = (50., 10., 1., 0.1)
# scale_in = (1., 1., 1., 1.)
dropping = 0.2
use_two = False
use_ib_lag = True  # 180 sec in Chemistry_BMS.py IB_LAG_CH
learning_rate = 0.0003
epochs = 500
hidden = 8
subsample = 5
nom_batch_size = 30
patience = 25
fail_ib_mag = 5
ib_bias = [.1, .5, 1.]
save_path = 'TrainTensorSatSoc_many_lstm.keras'

# Adjust data
train_attr, train_y = process_battery_attributes(train_attr, scale=scale_in)
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
train_dv_hys = train_attr[['dv_hys']]
validate_dv_hys = validate_attr[['dv_hys']]
test_dv_hys = test_attr[['dv_hys']]

# Adjust model automatically
if use_two:
    learning_rate *= 2
batch_size = int(nom_batch_size / subsample)

# Setup model
rows_train = int(len(train_y) / batch_size / subsample)
train_x = resizer(train_x, rows_train, batch_size, sub_samp=subsample, n_in=2)
train_x_vec = [train_x[:, :, 0], train_x[:, :, 1]]
train_y = resizer(train_y, rows_train, batch_size, sub_samp=subsample)
train_dv_hys = resizer(train_dv_hys, rows_train, batch_size, sub_samp=subsample)
rows_val = int(len(validate_y) / batch_size / subsample)
validate_x = resizer(validate_x, rows_val, batch_size, sub_samp=subsample, n_in=2)
validate_x_vec = [validate_x[:, :, 0], validate_x[:, :, 1]]
validate_y = resizer(validate_y, rows_val, batch_size, sub_samp=subsample)
validate_dv_hys = resizer(validate_dv_hys, rows_val, batch_size, sub_samp=subsample)
rows_tst = int(len(test_y) / batch_size / subsample)
test_x = resizer(test_x, rows_tst, batch_size, sub_samp=subsample, n_in=2)
test_x_vec = [test_x[:, :, 0], test_x[:, :, 1]]
test_y = resizer(test_y, rows_tst, batch_size, sub_samp=subsample)
test_dv_hys = resizer(test_dv_hys, rows_tst, batch_size, sub_samp=subsample)
model = tensor_model_create_many_to_one(hidden_units=hidden, input_shape=(train_x.shape[1], train_x.shape[2]),
                                        dense_units=1, activation=['relu', 'sigmoid', 'linear', 'linear'],
                                        learn_rate=learning_rate, drop=dropping, use_two_lstm=use_two, use_mae=fit_mae)

# Train model
print("[INFO] training model...")
epochs, mse, history = train_model(model, train_x, train_y, epochs_=epochs, btch_size=batch_size, verbose=1,
                                   patient=patience)
model.save(save_path)
plot_the_loss_curve(epochs, mse, history["loss"])

# make predictions
print("[INFO] predicting 'dv'...")
train_predict = model.predict(train_x)
validate_predict = model.predict(validate_x)
test_predict = model.predict(test_x)
train_x_fail = train_x.copy()
train_predict_fail_ib = []
for fail_ib_mag in ib_bias:
    train_x_fail[:, :, 0] += fail_ib_mag/scale_in[1]
    train_predict_fail_ib.append(model.predict(train_x_fail))

# Plot result
t_samp = train['cTime'][20]-train['cTime'][19]
t_samp_input = t_samp * subsample
t_samp_result = t_samp_input * batch_size
plot_input(trn_x=train_x, val_x=validate_x, tst_x=test_x, t_samp_=t_samp_input, use_ib_lag_=use_ib_lag)
plot_result(trn_y=train_y[:, batch_size-1, :], val_y=validate_y[:, batch_size-1, :], tst_y=test_y[:, batch_size-1, :],
            trn_pred=train_predict[:, batch_size-1, :], val_pred=validate_predict[:, batch_size-1, :],
            tst_pred=test_predict[:, batch_size-1, :],
            trn_dv_hys=train_dv_hys[:, batch_size-1, :], val_dv_hys=validate_dv_hys[:, batch_size-1,:],
            tst_dv_hys=test_dv_hys[:, batch_size-1, :],
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
classifier = load_model('TrainTensor_lstm.h5')
print(classifier.summary())

# Print error
print_error(trn_y=train_y[:, batch_size-1, :], val_y=validate_y[:, batch_size-1, :], tst_y=test_y[:, batch_size-1, :],
            trn_pred=train_predict[:, batch_size-1, :], val_pred=validate_predict[:, batch_size-1, :], tst_pred=test_predict[:, batch_size-1, :])

plt.show()

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
