#!/usr/bin/python3

"""
python3 -m pip install tensorflow-macos

This short introduction uses Keras to:
1. Load pre-build dataset.
2. Build a neural network machine learning model that classifies images.
3. Train this neural network.
4. Evaluate the accuracy of the model.
"""

import tensorflow as tf

if __name__ == '__main__':
    print("TensorFlow version: ", tf.__version__)

    # Load and prepare the MINIST dataset.
    # Convert the sample data from integers to floating-point numbers.
    # The MINIST database of handwritten digits, has a training set of 60,000 examples,
    # and a test of 10,000 examples.
    mnist = tf.keras.datasets.mnist
    (x_train, y_train), (x_test, y_test) = mnist.load_data()
    x_train, x_test = x_train / 255.0, x_test / 255.0

    # Build a tf.keras.Sequential model by stacking layers.
    model = tf.keras.models.Sequential([
        tf.keras.layers.Flatten(input_shape=(28, 28)),
        tf.keras.layers.Dense(128, activation='relu'),
        tf.keras.layers.Dropout(0.2),
        tf.keras.layers.Dense(10)
    ])

    # [0, 1)
    predictions = model(x_train[:1]).numpy()
    print(predictions)

    print(tf.nn.softmax(predictions).numpy())

    # Define a loss function for training.
    loss_fn = tf.keras.losses.SparseCategoricalCrossentropy(from_logits=True)
    print(loss_fn(y_train[:1], predictions).numpy())

    model.compile(optimizer='adam',
                  loss=loss_fn,
                  metrics=['accuracy'])

    model.fit(x_train, y_train, epochs=5)

    model.evaluate(x_test, y_test, verbose=2)

    probability_model = tf.keras.Sequential([
        model,
        tf.keras.layers.Softmax()
    ])

    print(probability_model(x_test[:5]))
