import numpy as np
from scipy.optimize import minimize

# known coordinates and depth of 3 master nodes
# coordinates from topis /floater/gnss
# since on surface depth is set to 0
depth = 0
node1 = {'lat': 0, 'lon': 0, 'depth': depth}
node2 = {'lat': 3, 'lon': 0, 'depth': depth}
node3 = {'lat': 0, 'lon': 4, 'depth': depth}

# target node depth in meters can be measuerd
target_depth = 2

speed_of_sound = 1500.0 # in m/s

# difference in time can be calculated from toa= current_time - target_sent_time in sec
toa1 = 0.01
toa2 = 0.02
toa3 = 0.03


# Function to calculate the difference between observed and estimated TOA
def objective_function(target_coordinates):
    lat, lon = target_coordinates
    estimated_toa1 = (np.sqrt(
        (node1['lat'] - lat) ** 2 + (node1['lon'] - lon) ** 2 + (node1['depth'] - target_depth) ** 2)) / speed_of_sound
    estimated_toa2 = (np.sqrt(
        (node2['lat'] - lat) ** 2 + (node2['lon'] - lon) ** 2 + (node2['depth'] - target_depth) ** 2)) / speed_of_sound
    estimated_toa3 = (np.sqrt(
        (node3['lat'] - lat) ** 2 + (node3['lon'] - lon) ** 2 + (node3['depth'] - target_depth) ** 2)) / speed_of_sound

    error = [
        estimated_toa1 - toa1,
        estimated_toa2 - toa2,
        estimated_toa3 - toa3
    ]

    return np.sum(np.square(error))


# Initial guess
initial_guess = [node1['lat'], node1['lon']]

# Perform optimization to find the target coordinates that minimize the TOA error
result = minimize(objective_function, initial_guess, method='Nelder-Mead')

if result.success:
    estimated_target_coordinates = result.x
    print("Estimated Target Coordinates (Latitude, Longitude):", estimated_target_coordinates)
else:
    print("Optimization failed. Could not determine target coordinates.")
