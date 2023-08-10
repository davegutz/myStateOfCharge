# sunspot RNN example
# from https://machinelearningmastery.com/understanding-simple-recurrent-neural-networks-in-keras/

from pandas import read_csv
import numpy as np
from keras.models import Sequential
from keras.layers import Dense, SimpleRNN, LSTM
from sklearn.preprocessing import MinMaxScaler
from sklearn.metrics import mean_squared_error
import math
import matplotlib.pyplot as plt


def get_train_test(url, split_percent=0.8):
    df = read_csv(url, usecols=[1], engine='python')
    data = np.array(df.values.astype('float32'))
    scaler = MinMaxScaler(feature_range=(0, 1))
    data = scaler.fit_transform(data).flatten()
    n = len(data)
    # Point for splitting data into train and test
    split = int(n * split_percent)
    train_data = data[range(split)]
    test_data = data[split:]
    return train_data, test_data, data


# Prepare the input X and target Y
def get_XY(dat, time_steps):
    Y_ind = np.arange(time_steps, len(dat), time_steps)
    Y = dat[Y_ind]
    rows_x = len(Y)
    X = dat[range(time_steps * rows_x)]
    X = np.reshape(X, (rows_x, time_steps, 1))
    return X, Y


def get_XYol(dat, time_steps):
    Y_ind = np.arange(1, len(dat), 1)
    Y = dat[Y_ind]
    rows_x = len(Y) + 1
    n_ol = rows_x - time_steps
    X = np.zeros((n_ol, time_steps, 1))
    for i in range(n_ol):
        iend = i + time_steps
        slice = np.reshape(train_data[i:iend], (1, time_steps, 1))
        # print(i, ':', iend, '=>', slice)
        X[i] = slice
    return X, Y


def create_LSTM(hidden_units, dense_units, input_shape, activation):
    model = Sequential()
    model.add(LSTM(hidden_units, input_shape=input_shape, activation=activation[0]))
    model.add(Dense(units=dense_units, activation=activation[1]))
    model.compile(loss='mean_squared_error', optimizer='adam')
    return model


def print_error(trainY, testY, train_predict, test_predict):
    # Error of predictions
    train_rmse = math.sqrt(mean_squared_error(trainY, train_predict))
    test_rmse = math.sqrt(mean_squared_error(testY, test_predict))
    # Print RMSE
    print('Train RMSE: %.3f RMSE' % (train_rmse))
    print('Test RMSE: %.3f RMSE' % (test_rmse))


# Plot the result
def plot_result(trainY, testY, train_predict, test_predict, train_data, test_data, train_data_predict, test_data_predict):
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

    actual = np.append(train_data, test_data)
    predictions = np.append(train_data_predict, test_data_predict)
    rows = len(actual)
    plt.figure(figsize=(15, 6), dpi=80)
    plt.plot(range(rows), actual)
    plt.plot(range(rows), predictions)
    plt.axvline(x=len(train_data), color='r')
    plt.legend(['Actual', 'Predictions'])
    plt.xlabel('Observation number after given time steps')
    plt.ylabel('Sunspots scaled')
    plt.title('Actual and Predicted Values. The Red Line Separates The Training And Test Examples')

sunspots_url = 'https://raw.githubusercontent.com/jbrownlee/Datasets/master/monthly-sunspots.csv'
time_steps = 12
train_data, test_data, data = get_train_test(sunspots_url)
trainX, trainY = get_XY(train_data, time_steps)
testX, testY = get_XY(test_data, time_steps)
trainXol, trainYol = get_XY(train_data, 1)
testXol, testYol = get_XY(test_data, 1)
trainXolx, trainYolx = get_XYol(train_data, time_steps)
testXolx, testYolx = get_XYol(test_data, time_steps)

# Create model and train
model = create_LSTM(hidden_units=3, dense_units=1, input_shape=(time_steps, 1), activation=['tanh', 'tanh'])
model.fit(trainX, trainY, epochs=20, batch_size=1, verbose=2)

# make predictions
train_predict = model.predict(trainX)
test_predict = model.predict(testX)

# overlapping predictions; 'ol' = overlapping
n = train_data.shape[0]
n_ol = n - time_steps
trainXol_ = np.zeros((n_ol, time_steps, 1))
for i in range(n_ol):
    slice = np.reshape(train_data[i:i+time_steps], (1, time_steps, 1))
    trainXol_[i] = slice
n = test_data.shape[0]
n_ol = n - time_steps
testXol_ = np.zeros((n_ol, time_steps, 1))
for i in range(n_ol):
    slice = np.reshape(test_data[i:i+time_steps], (1, time_steps, 1))
    testXol_[i] = slice

train_predictol = model.predict(trainXol_)
test_predictol = model.predict(testXol_)

# Print error
print_error(trainY, testY, train_predict, test_predict)

# Plot result
tsQ2 = int(time_steps / 2)
plot_result(trainY, testY, train_predict, test_predict, trainYol[tsQ2:-tsQ2],  testYol[tsQ2:-tsQ2], train_predictol[:-1], test_predictol[:-1])
plt.show()
