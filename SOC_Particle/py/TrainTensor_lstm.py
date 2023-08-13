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
from keras.layers import Dense, SimpleRNN, LSTM, Dropout
from sklearn.metrics import mean_squared_error
from sklearn.model_selection import train_test_split

# The following lines adjust the granularity of reporting.
pd.options.display.max_rows = 10
pd.options.display.float_format = "{:.1f}".format


def create_lstm(hidden_units, input_shape, dense_units=1, activation=None, learn_rate=0.01, drop=0.2,
                use_two_lstm=False):
    if activation is None:
        activation = ['relu', 'sigmoid', 'linear']
    lstm = Sequential()
    if use_two_lstm:
        lstm.add(LSTM(hidden_units, input_shape=input_shape, activation=activation[0],
                      recurrent_activation=activation[1], return_sequences=True))
    else:
        lstm.add(LSTM(hidden_units, input_shape=input_shape, activation=activation[0],
                      recurrent_activation=activation[1], return_sequences=False))
    lstm.add(Dropout(drop))
    if use_two_lstm:
        lstm.add(LSTM(hidden_units, activation=activation[0],
                      recurrent_activation=activation[1], return_sequences=False))
        lstm.add(Dropout(drop))
    lstm.add(Dense(units=dense_units, activation=activation[2]))
    # lstm.compile(loss='mean_squared_error', optimizer='adam')
    lstm.compile(loss='mean_squared_error', optimizer=tf.keras.optimizers.Adam(learning_rate=learn_rate))
    lstm.summary()
    return lstm


# Plotting function
def plot_the_loss_curve(epochs_, mse_training, mse_validation):
    """Plot a curve of loss vs. epoch."""
    plt.figure()
    plt.xlabel("Epoch")
    plt.ylabel("Mean Squared Error")
    plt.plot(epochs_, mse_training, label="Training Loss")
    plt.plot(epochs_, mse_validation, label="Validation Loss")

    # mse_training is a pandas Series, so convert it to a list first.
    merged_mse_lists = np.append(mse_training.to_numpy(), mse_validation)
    highest_loss = max(merged_mse_lists)
    lowest_loss = min(merged_mse_lists)
    top_of_y_axis = highest_loss * 1.03
    bottom_of_y_axis = lowest_loss * 0.97

    plt.ylim([bottom_of_y_axis, top_of_y_axis])
    plt.legend()


# Plot the inputs
def plot_input(trn_x, val_x, tst_x):
    inp = np.append(trn_x, val_x)
    inp = np.append(inp, tst_x)
    rows = len(inp)
    plt.figure(figsize=(15, 6), dpi=80)
    plt.plot(range(rows), inp)
    plt.axvline(x=len(trn_x), color='r')
    plt.axvline(x=len(trn_x)+len(val_x), color='r')
    plt.legend(['Inputs'])
    plt.xlabel('Observation number after given time steps')
    plt.ylabel('ib scaled')
    plt.title('Inputs.  The Red Line Separates The Training, Validation, and Test Examples')


# Plot the results
def plot_result(trn_y, val_y, tst_y, trn_pred, val_pred, tst_pred):
    actual = np.append(trn_y, val_y)
    actual = np.append(actual, tst_y)
    predictions = np.append(trn_pred, val_pred)
    predictions = np.append(predictions, tst_pred)
    rows = len(actual)
    plt.figure(figsize=(15, 6), dpi=80)
    plt.plot(range(rows), actual)
    plt.plot(range(rows), predictions)
    plt.axvline(x=len(trn_y), color='r')
    plt.axvline(x=len(trn_y)+len(val_y), color='r')
    plt.legend(['Actual', 'Predictions'])
    plt.xlabel('Observation number after given time steps')
    plt.ylabel('dv scaled')
    plt.title('Actual and Predicted Values. The Red Line Separates The Training, Validation, and Test Examples')


def print_error(trn_y, val_y, tst_y, trn_pred, val_pred, tst_pred):
    # Error of predictions
    trn_rmse = math.sqrt(mean_squared_error(trn_y, trn_pred))
    val_rmse = math.sqrt(mean_squared_error(val_y, val_pred))
    tst_rmse = math.sqrt(mean_squared_error(tst_y, tst_pred))

    # Print RMSE
    print('Train RMSE: %.3f RMSE' % trn_rmse)
    print('Validate RMSE: %.3f RMSE' % val_rmse)
    print('Test RMSE: %.3f RMSE' % tst_rmse)


def process_battery_attributes(df):
    # column names of continuous features
    data = df[['Tb', 'ib', 'soc']]
    with pd.option_context('mode.chained_assignment', None):
        data['Tb'] = data['Tb'].map(lambda x: x/50.)
        data['ib'] = data['ib'].map(lambda x: x/10.)
        data['soc'] = data['soc'].map(lambda x: x/1.)
        y = df['dv'] / 0.1

    return data, y


# Resize and convert to numpy
def resizer(v, targ_rows, btch_size, sub_samp=1):
    v = v.to_numpy('float')[:targ_rows*btch_size*sub_samp]
    rows_raw = len(v)
    x = v.reshape(int(rows_raw/sub_samp), sub_samp, 1)[:, 0, 0]
    return x.reshape(targ_rows, btch_size, 1)


def train_model(mod, x, y, epochs_=20, btch_size=1, verbose=0):
    """Feed a dataset into the model in order to train it."""

    # train the model
    hist = mod.fit(x=x, y=y, epochs=epochs_, batch_size=btch_size, shuffle=False, verbose=verbose)

    # Get details that will be useful for plotting the loss curve.
    epochs_ = hist.epoch
    ret_hist = pd.DataFrame(hist.history)
    mserr = ret_hist["loss"]
    return epochs_, mserr, ret_hist


# Load data and normalize
# train_file = ".//temp//dv_train_soc0p_ch_rep.csv"
# test_file = ".//temp//dv_test_soc0p_ch_rep.csv"
train_file = ".//generateDV_Data.csv"
val_fraction = 0.2
test_file = ".//generateDV_Data.csv"

print("[INFO] loading train/validation attributes...")
train = pd.read_csv(train_file, skipinitialspace=True)
train['voc'] = train['vb'] - train['dv_dyn']
train['dv'] = train['voc'] - train['voc_stat']
train_df = train[['Tb', 'ib', 'soc', 'dv']]

print("[INFO] loading test attributes...")
test = pd.read_csv(".//generateDV_Data.csv", skipinitialspace=True)
test['voc'] = test['vb'] - test['dv_dyn']
test['dv'] = test['voc'] - test['voc_stat']
test_df = test[['Tb', 'ib', 'soc', 'dv']]

# Split training data into train and validation data frames
train_attr, validate_attr = train_test_split(train_df, test_size=val_fraction, shuffle=False)
test_attr, test_val_attr = train_test_split(test_df, test_size=None, shuffle=False)
train_attr, train_y = process_battery_attributes(train_attr)
validate_attr, validate_y = process_battery_attributes(validate_attr)
test_attr, test_y = process_battery_attributes(test_attr)
train_x = train_attr['ib']
validate_x = validate_attr['ib']
test_x = test_attr['ib']

# Create model
dropping = 0.2
use_two = False
learning_rate = 0.0001
epochs = 50
hidden = 4
subsample = 5
nom_batch_size = 30

# Adjust model automatically
if use_two:
    learning_rate *= 2
batch_size = int(nom_batch_size / subsample)

# Setup model
rows_train = int(len(train_y) / batch_size / subsample)
train_x = resizer(train_x, rows_train, batch_size, sub_samp=subsample)
train_y = resizer(train_y, rows_train, batch_size, sub_samp=subsample)
rows_val = int(len(validate_y) / batch_size / subsample)
validate_x = resizer(validate_x, rows_val, batch_size, sub_samp=subsample)
validate_y = resizer(validate_y, rows_val, batch_size, sub_samp=subsample)
rows_tst = int(len(test_y) / batch_size / subsample)
test_x = resizer(test_x, rows_tst, batch_size, sub_samp=subsample)
test_y = resizer(test_y, rows_tst, batch_size, sub_samp=subsample)
model = create_lstm(hidden_units=hidden, input_shape=(train_x.shape[1], train_x.shape[2]), dense_units=1,
                    activation=['relu', 'sigmoid', 'linear'], learn_rate=learning_rate, drop=dropping,
                    use_two_lstm=use_two)

# Train model
print("[INFO] training model...")
epochs, mse, history = train_model(model, train_x, train_y, epochs_=epochs, btch_size=batch_size, verbose=1)
plot_the_loss_curve(epochs, mse, history["loss"])

# make predictions
print("[INFO] predicting 'dv'...")
train_predict = model.predict(train_x)
validate_predict = model.predict(validate_x)
test_predict = model.predict(test_x)

# Print error
print_error(trn_y=train_y[:, batch_size-1, :], val_y=validate_y[:, batch_size-1, :], tst_y=test_y[:, batch_size-1, :],
            trn_pred=train_predict, val_pred=validate_predict, tst_pred=test_predict)

# Plot result
plot_input(trn_x=train_x, val_x=validate_x, tst_x=test_x)
plot_result(trn_y=train_y[:, batch_size-1, :], val_y=validate_y[:, batch_size-1, :], tst_y=test_y[:, batch_size-1, :],
            trn_pred=train_predict, val_pred=validate_predict, tst_pred=test_predict)

# Print model
model.summary()
print('\nShape:')
for wt in model.layers[0].weights:
    print(wt.name, '-->', wt.shape)

plt.show()


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
