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

import numpy as np
import pandas as pd
import tensorflow as tf
from matplotlib import pyplot as plt
import seaborn as sns

# The following lines adjust the granularity of reporting.
pd.options.display.max_rows = 10
pd.options.display.float_format = "{:.1f}".format


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


# Create linear regression
def create_model(my_inputs, my_outputs, my_learning_rate):
    """Create and compile a simple linear regression model."""
    model = tf.keras.Model(inputs=my_inputs, outputs=my_outputs)

    # Construct the layers into a model that TensorFlow can execute.
    model.compile(optimizer=tf.keras.optimizers.Adam(learning_rate=my_learning_rate),
                  loss="mean_squared_error", metrics=[tf.keras.metrics.MeanSquaredError()])
    return model


def train_model(model, dataset, epochs, batch_size, label_name, df, validation_split=0.1):
    """Feed a dataset into the model in order to train it."""

    # Create Normalization layers to normalize the median_house_value data.
    # Because median_house_value is our label (i.e., the target value we're
    # predicting), these layers won't be added to our model.
    train_dv_normalized = tf.keras.layers.Normalization(axis=None)
    train_dv_normalized.adapt(
        np.array(df['dv']))

    # Split the dataset into features and label.
    features = {name:np.array(value) for name, value in dataset.items()}
    label = train_dv_normalized(np.array(features.pop(label_name)))
    history = model.fit(x=features, y=label, batch_size=batch_size, epochs=epochs, shuffle=False,
                        validation_split=validation_split)

    # Get details that will be useful for plotting the loss curve.
    epochs = history.epoch
    hist = pd.DataFrame(history.history)
    mse = hist["mean_squared_error"]
    return epochs, mse, history.history


# Load data
train_df = pd.read_csv(".//temp//dv_train_soc0p_ch_rep.csv", skipinitialspace =True)
# test_df = pd.read_csv("https://download.mlcc.google.com/mledu-datasets/california_housing_test.csv")
train_df['dv'] = train_df['voc_stat'] - train_df['voc_soc']

# Keras Input tensors of float values.
inputs = {
    'Tb':
        tf.keras.layers.Input(shape=(1,), dtype=tf.float32,
                              name='Tb'),
    'vb':
        tf.keras.layers.Input(shape=(1,), dtype=tf.float32,
                              name='vb'),
    'ib':
        tf.keras.layers.Input(shape=(1,), dtype=tf.float32,
                              name='ib'),
    'soc':
        tf.keras.layers.Input(shape=(1,), dtype=tf.float32,
                              name='soc'),
    'dv':
        tf.keras.layers.Input(shape=(1,), dtype=tf.float32,
                              name='dv'),
}

# Create a Normalization layer to normalize the soc
soc = tf.keras.layers.Normalization(
    name='normalization_soc',
    axis=None)
soc.adapt(train_df['soc'])
soc = soc(inputs.get('soc'))

# Create a Normalization layer to normalize the dv
ib = tf.keras.layers.Normalization(
    name='normalization_ib',
    axis=None)
ib.adapt(train_df['ib'])
ib = ib(inputs.get('ib'))

# Create a Normalization layer to normalize the dv
dv = tf.keras.layers.Normalization(
    name='normalization_dv',
    axis=None)
dv.adapt(train_df['dv'])
dv = dv(inputs.get('dv'))

