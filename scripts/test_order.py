import numpy as np

for _ in range(100):

  tids = np.arange(8)
  np.random.shuffle(tids)

  order = np.zeros_like(tids)
  order[np.arange(8)] = tids

  fixed = tids[np.argsort(order)]
  output = fixed[np.argsort(np.argsort(order))]

  assert np.array_equal(fixed, np.arange(8))
  assert np.array_equal(tids, output)