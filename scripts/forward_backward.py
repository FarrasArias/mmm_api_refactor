import numpy as np

order = np.arange(5)
np.random.shuffle(order)

print(order)

print( np.argsort(order) )