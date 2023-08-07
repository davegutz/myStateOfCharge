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
from keras.layers import Dense, SimpleRNN, LSTM
import seaborn as sns

# The following lines adjust the granularity of reporting.
pd.options.display.max_rows = 10
pd.options.display.float_format = "{:.1f}".format


def create_LSTM(hidden_units, dense_units, input_shape, activation, my_learning_rate):
    """
    :param hidden_units:
    :param dense_units:
    :param input_shape:
    :param activation:
    :return:
    """
    return model


# Plotting function
def plot_the_loss_curve(epochs, mse_training, mse_validation):
    """Plot a curve of loss vs. epoch."""

    plt.figure()
    plt.xlabel("Epoch")
    plt.ylabel("Mean Squared Error")

    plt.plot(epochs, mse_training, label="Training Loss")
    plt.plot(epochs, mse_validation, label="Validation Loss")

    # mse_training is a pandas Series, so convert it to a list first.
    merged_mse_lists = mse_training.tolist() + mse_validation
    highest_loss = max(merged_mse_lists)
    lowest_loss = min(merged_mse_lists)
    top_of_y_axis = highest_loss * 1.03
    bottom_of_y_axis = lowest_loss * 0.97

    plt.ylim([bottom_of_y_axis, top_of_y_axis])
    plt.legend()
    plt.show()


# Plot the result
def plot_result(trainY, testY, train_predict, test_predict):
    actual = np.append(trainY, testY)
    predictions = np.append(train_predict, test_predict)
    rows = len(actual)
    plt.figure(figsize=(15, 6), dpi=80)
    plt.plot(range(rows), actual)
    plt.plot(range(rows), predictions)
    plt.axvline(x=len(trainY), color='r')
    plt.legend(['Actual', 'Predictions'])
    plt.xlabel('Observation number after given time steps')
    plt.ylabel('Sunspots scaled')
    plt.title('Actual and Predicted Values. The Red Line Separates The Training And Test Examples')



def print_error(trainY, testY, train_predict, test_predict):
    # Error of predictions
    train_rmse = math.sqrt(mean_squared_error(trainY, train_predict))
    test_rmse = math.sqrt(mean_squared_error(testY, test_predict))
    # Print RMSE
    print('Train RMSE: %.3f RMSE' % (train_rmse))
    print('Test RMSE: %.3f RMSE' % (test_rmse))


def train_model(model, dataset, epochs, batch_size, label_name, df, validation_split=0.1, verbose=0):
    """Feed a dataset into the model in order to train it."""

    # Create Normalization layers to normalize the median_house_value data.
    # Because median_house_value is our label (i.e., the target value we're
    # predicting), these layers won't be added to our model.
    train_dv_normalized = tf.keras.layers.Normalization(axis=None)
    train_dv_normalized.adapt(np.array(df[label_name]))

    # Split the dataset into features and label.
    features = {name:np.array(value) for name, value in dataset.items()}
    label = train_dv_normalized(np.array(features.pop(label_name)))
    history = model.fit(x=features, y=label, batch_size=batch_size, epochs=epochs, shuffle=False,
                        validation_split=validation_split, verbose=2)

    # Get details that will be useful for plotting the loss curve.
    epochs = history.epoch
    hist = pd.DataFrame(history.history)
    mse = hist["mean_squared_error"]
    return epochs, mse, history.history


# Load data and normalize
train_df = pd.read_csv(".//temp//dv_train_soc0p_ch_rep.csv", skipinitialspace =True)
test_df = pd.read_csv(".//temp//dv_test_soc0p_ch_rep.csv", skipinitialspace =True)

train_df['Tb'] = train_df['Tb'] / 50.
train_df['ib'] = train_df['ib'] / 50.
train_df['soc'] = train_df['soc'] / 1.
train_df['dv'] = (train_df['voc_soc'] - train_df['voc_stat']) / 5.
trainX = [[],[]]
trainX[0][:] = train_df['ib']
trainX[1][:] = train_df['soc']
trainY = train_df['dv']

test_df['Tb'] = test_df['Tb'] / 50.
Tb_boundaries = np.linspace(0, 50, 3)
test_df['ib'] = test_df['ib'] / 50.
test_df['soc'] = test_df['soc'] / 1.
test_df['dv'] = (test_df['voc_soc'] - test_df['voc_stat']) / 5.
testX = [[],[]]
testX[0][:] = test_df['ib']
testX[1][:] = test_df['soc']
testY = test_df['dv']

inputs = {
    'Tb':
        tf.keras.layers.Input(shape=(1,), dtype=tf.float32, name='Tb'),
    'ib':
        tf.keras.layers.Input(shape=(1,), dtype=tf.float32, name='ib'),
    'soc':
        tf.keras.layers.Input(shape=(1,), dtype=tf.float32, name='soc'),
}

Tb = tf.keras.layers.Normalization(name='Tb_norm', axis=None)
Tb.adapt(train_df['Tb'])
Tb = Tb(inputs.get('Tb'))
Tb = tf.keras.layers.Discretization(bin_boundaries=Tb_boundaries, name='discretization_Tb')(Tb)

ib = tf.keras.layers.Normalization(name='ib_norm', axis=None)
ib.adapt(train_df['ib'])
ib = ib(inputs.get('ib'))

soc = tf.keras.layers.Normalization(name='soc_norm', axis=None)
soc.adapt(train_df['soc'])
soc = soc(inputs.get('soc'))


# Create model and train
time_steps = 12
hidden_units = 3
learning_rate = 0.01
epochs = 20
batch_size = 1
validation_split = 0.2
model = Sequential()
input_layer = tf.keras.layers.Concatenate()(
    [ib, soc])
model.add(LSTM(hidden_units, input_shape=(time_steps, 2), activation='relu'))(input_layer)
model.add(Dense(units=1, activation='relu'))
model.compile(loss='mean_squared_error', optimizer=tf.keras.optimizers.Adam(learning_rate=learning_rate),
              metrics=[tf.keras.metrics.MeanSquaredError()])
epochs, mse, history = train_model(model, train_df, epochs, batch_size, label_name='dv', df=train_df,
                                   validation_split=validation_split, verbose=2)
plot_the_loss_curve(epochs, mse, history["val_mean_squared_error"])

# make predictions
train_predict = model.predict(trainX)
test_predict = model.predict(testX)

# Print error
print_error(trainY, testY, train_predict, test_predict)

# Plot result
plot_result(trainY, testY, train_predict, test_predict)
plt.show()
