=============
pymintriangle
=============

A python wrapper for the minimal area enclosing triangle algorithm available here_, which was included in OpenCV_. This wrapper is adapted from the OpenCV implementation.

.. _here: https://github.com/ovidiuparvu/minimal-area-triangle
.. _OpenCV: https://github.com/opencv/opencv/blob/master/modules/imgproc/src/min_enclosing_triangle.cpp

Prerequisites
=============

- python >= 3.6
- pybind11 >= 2.3.0

Install
=======

.. code-block:: sh

    pip install .

Usage
=====

In python import the module and use the compute method to compute the minimal area enclosing triangle.

.. code-block:: python

    import numpy as np
    import pymintriangle
    from scipy.spatial import ConvexHull

    points = np.random.rand(20, 2)
    convex_hull = ConvexHull(points)
    triangle = pymintriangle.compute(points[convex_hull.vertices])

Documentation
=============

compute
~~~~~~~

Computes the minimal area triangle of set of points making a convex hull.

Parameters
----------
convexHull : (Mx2) array
    The coordinates of the points making the convex hull.

Returns
-------
triangle : (3x2) array
    The coordinates of the points of the triangle.
