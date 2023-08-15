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
from sklearn.metrics import mean_squared_error
from sklearn.model_selection import train_test_split
from keras.callbacks import EarlyStopping
from keras.callbacks import ModelCheckpoint

# The following lines adjust the granularity of reporting.
pd.options.display.max_rows = 10
pd.options.display.float_format = "{:.1f}".format


# Single input with one-hot augmentation path
def create_sat_mod(hidden_units, input_shape, dense_units=1, activation=None, learn_rate=0.01, drop=0.2,
                use_two_lstm=False):
    if activation is None:
        activation = ['relu', 'sigmoid', 'linear', 'linear']
    simple_input = (input_shape[0], 1)
    ib_in = Input(shape=simple_input, name='ib-in')
    sat_in = Input(shape=simple_input, name='sat-in')

    # LSTM using ib
    lstm = Sequential()
    lstm.add(ib_in)
    if use_two_lstm:
        lstm.add(LSTM(hidden_units, input_shape=simple_input, activation=activation[0],
                      recurrent_activation=activation[1], return_sequences=True))
    else:
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
    merged = concatenate([lstm.output, sat_in])
    predictions = Dense(units=dense_units, activation=activation[2])(merged)
    final = Model(inputs=[ib_in, sat_in], outputs=predictions)
    final.compile(loss='mean_squared_error', optimizer=tf.keras.optimizers.Adam(learning_rate=learn_rate))
    final.summary()

    return final


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
    inp_ib = np.append(trn_x[:, :, 0], val_x[:, :, 0])
    inp_ib = np.append(inp_ib, tst_x[:, :, 0])
    inp_sat = np.append(trn_x[:, :, 1], val_x[:, :, 1])
    inp_sat = np.append(inp_sat, tst_x[:, :, 1])
    rows = len(inp_ib)  # inp is unstacked while all the x's are stacked
    plt.figure(figsize=(15, 6), dpi=80)
    plt.plot(range(rows), inp_ib,  label='ib scaled')
    plt.plot(range(rows), inp_sat, label='sat')
    trn_len = trn_x.shape[0]*trn_x.shape[1]  # x's are stacked
    val_len = val_x.shape[0]*val_x.shape[1]  # x's are stacked
    plt.axvline(x=trn_len, color='r')
    plt.axvline(x=trn_len+val_len, color='r')
    plt.legend(loc=3)
    plt.xlabel('Observation number after given time steps')
    plt.ylabel('ib scaled / sat')
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
    data = df[['Tb', 'ib', 'soc', 'sat']]
    with pd.option_context('mode.chained_assignment', None):
        data['Tb'] = data['Tb'].map(lambda x: x/50.)
        data['ib'] = data['ib'].map(lambda x: x/10.)
        data['soc'] = data['soc'].map(lambda x: x/1.)
        data['sat'] = data['sat'].map(lambda x: x/1.)
        y = df['dv'] / 0.1

    return data, y


# Resize and convert to numpy
def resizer(v, targ_rows, btch_size, sub_samp=1, n_in=1):
    v = v.to_numpy('float')[:targ_rows*btch_size*sub_samp]
    rows_raw = len(v)
    x = v.reshape(int(rows_raw/sub_samp), sub_samp, n_in)[:, 0, :]
    return x.reshape(targ_rows, btch_size, n_in)


def train_model(mod, x, y, epochs_=20, btch_size=1, verbose=0, patient=10):
    """Feed a dataset into the model in order to train it."""

    # Training callbacks
    es = EarlyStopping(monitor='loss', mode='min', verbose=1, patience=patient)
    file_path = 'TrainTensor_lstm.h5'
    mc = ModelCheckpoint(file_path, monitor='loss', mode='min', verbose=1, save_weights_only=False, save_best_only=True)
    cb = [es, mc]

    # train the model
    hist = mod.fit(x=x, y=y, epochs=epochs_, batch_size=btch_size, shuffle=False, verbose=verbose, callbacks=cb)

    # Get details that will be useful for plotting the loss curve.
    epochs_ = hist.epoch
    ret_hist = pd.DataFrame(hist.history)
    mserr = ret_hist["loss"]
    return epochs_, mserr, ret_hist


# Load data and normalize
# train_file = ".//temp//dv_train_soc0p_ch_rep.csv"
# test_file = ".//temp//dv_test_soc0p_ch_rep.csv"
train_file = ".//generateDV_Data.csv"
val_fraction = 0.25
test_file = ".//generateDV_Data.csv"

print("[INFO] loading train/validation attributes...")
train = pd.read_csv(train_file, skipinitialspace=True)
train['voc'] = train['vb'] - train['dv_dyn']
train['dv'] = train['voc'] - train['voc_stat']
train_df = train[['Tb', 'ib', 'soc', 'sat', 'dv']]

print("[INFO] loading test attributes...")
test = pd.read_csv(".//generateDV_Data.csv", skipinitialspace=True)
test['voc'] = test['vb'] - test['dv_dyn']
test['dv'] = test['voc'] - test['voc_stat']
test_df = test[['Tb', 'ib', 'soc', 'sat', 'dv']]

# Split training data into train and validation data frames
train_attr, validate_attr = train_test_split(train_df, test_size=val_fraction, shuffle=False)
test_attr, test_val_attr = train_test_split(test_df, test_size=None, shuffle=False)
train_attr, train_y = process_battery_attributes(train_attr)
validate_attr, validate_y = process_battery_attributes(validate_attr)
test_attr, test_y = process_battery_attributes(test_attr)
train_x = train_attr[['ib', 'sat']]
validate_x = validate_attr[['ib', 'sat']]
test_x = test_attr[['ib', 'sat']]


# Create model
dropping = 0.2
use_two = False
learning_rate = 0.001
epochs = 250
hidden = 12
subsample = 5
nom_batch_size = 30
patience = 5

# Adjust model automatically
if use_two:
    learning_rate *= 2
batch_size = int(nom_batch_size / subsample)

# Setup model
rows_train = int(len(train_y) / batch_size / subsample)
train_x = resizer(train_x, rows_train, batch_size, sub_samp=subsample, n_in=2)
train_x_vec = [train_x[:, :, 0], train_x[:, :, 1]]
train_y = resizer(train_y, rows_train, batch_size, sub_samp=subsample)
rows_val = int(len(validate_y) / batch_size / subsample)
validate_x = resizer(validate_x, rows_val, batch_size, sub_samp=subsample, n_in=2)
validate_x_vec = [validate_x[:, :, 0], validate_x[:, :, 1]]
validate_y = resizer(validate_y, rows_val, batch_size, sub_samp=subsample)
rows_tst = int(len(test_y) / batch_size / subsample)
test_x = resizer(test_x, rows_tst, batch_size, sub_samp=subsample, n_in=2)
test_x_vec = [test_x[:, :, 0], test_x[:, :, 1]]
test_y = resizer(test_y, rows_tst, batch_size, sub_samp=subsample)
model = create_sat_mod(hidden_units=hidden, input_shape=(train_x.shape[1], train_x.shape[2]), dense_units=1,
                       activation=['relu', 'sigmoid', 'linear', 'linear'], learn_rate=learning_rate, drop=dropping,
                       use_two_lstm=use_two)

# Train model
print("[INFO] training model...")
epochs, mse, history = train_model(model, train_x_vec, train_y, epochs_=epochs, btch_size=batch_size, verbose=1,
                                   patient=patience)
plot_the_loss_curve(epochs, mse, history["loss"])

# make predictions
print("[INFO] predicting 'dv'...")
train_predict = model.predict(train_x_vec)
validate_predict = model.predict(validate_x_vec)
test_predict = model.predict(test_x_vec)

# Plot result
plot_input(trn_x=train_x, val_x=validate_x, tst_x=test_x)
plot_result(trn_y=train_y[:, batch_size-1, :], val_y=validate_y[:, batch_size-1, :], tst_y=test_y[:, batch_size-1, :],
            trn_pred=train_predict[:, batch_size-1, :], val_pred=validate_predict[:, batch_size-1, :], tst_pred=test_predict[:, batch_size-1, :])

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
