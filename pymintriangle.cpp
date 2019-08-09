/*M///////////////////////////////////////////////////////////////////////////////////////
//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
//  INFORMATION REGARDING THE CONTRIBUTION:
//
//  Author: Ovidiu Parvu
//  Affiliation: Brunel University
//  Created: 11.09.2013
//  E-mail: <ovidiu.parvu[AT]gmail.com>
//  Web: http://people.brunel.ac.uk/~cspgoop
//
//  These functions were implemented during Ovidiu Parvu's first year as a PhD student at
//  Brunel University, London, UK. The PhD project is supervised by prof. David Gilbert (principal)
//  and prof. Nigel Saunders (second).
//
//  THE IMPLEMENTATION OF THE MODULES IS BASED ON THE FOLLOWING PAPERS:
//
//  [1] V. Klee and M. C. Laskowski, "Finding the smallest triangles containing a given convex
//  polygon", Journal of Algorithms, vol. 6, no. 3, pp. 359-375, Sep. 1985.
//  [2] J. O'Rourke, A. Aggarwal, S. Maddila, and M. Baldwin, "An optimal algorithm for finding
//  minimal enclosing triangles", Journal of Algorithms, vol. 7, no. 2, pp. 258-269, Jun. 1986.
//
//  The overall complexity of the algorithm is theta(n) where "n" represents the number
//  of vertices in the convex polygon.
//
//
//
//                           License Agreement
//                For Open Source Computer Vision Library
//
// Copyright (C) 2000, Intel Corporation, all rights reserved.
// Copyright (C) 2013, OpenCV Foundation, all rights reserved.
// Copyright (C) 2013, Ovidiu Parvu, all rights reserved.
// Third party copyrights are property of their respective owners.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//   * Redistribution's of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//
//   * Redistribution's in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//   * The name of the copyright holders may not be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
// This software is provided by the copyright holders and contributors "as is" and
// any express or implied warranties, including, but not limited to, the implied
// warranties of merchantability and fitness for a particular purpose are disclaimed.
// In no event shall the Intel Corporation or contributors be liable for any direct,
// indirect, incidental, special, exemplary, or consequential damages
// (including, but not limited to, procurement of substitute goods or services;
// loss of use, data, or profits; or business interruption) however caused
// and on any theory of liability, whether in contract, strict liability,
// or tort (including negligence or otherwise) arising in any way out of
// the use of this software, even if advised of the possibility of such damage.
//
//M*/

#define _USE_MATH_DEFINES
#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>
#include <stdexcept>
#include <assert.h>

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
using namespace pybind11::literals;
namespace py = pybind11;

/////////////////////////////////// Struct definitions ///////////////////////////////////

struct Point {
    double x;
    double y;
};

///////////////////////////////// Constants definitions //////////////////////////////////


// Intersection of line and polygon

#define INTERSECTS_BELOW        1
#define INTERSECTS_ABOVE        2
#define INTERSECTS_CRITICAL     3

// Error messages

#define ERR_SIDE_B_GAMMA        "The position of side B could not be determined, because gamma(b) could not be computed."
#define ERR_VERTEX_C_ON_SIDE_B  "The position of the vertex C on side B could not be determined, because the considered lines do not intersect."

// Possible values for validation flag

#define VALIDATION_SIDE_A_TANGENT   0
#define VALIDATION_SIDE_B_TANGENT   1
#define VALIDATION_SIDES_FLUSH      2

// Threshold value for comparisons

#define EPSILON 1E-5


////////////////////////////// Helper functions declarations /////////////////////////////

namespace pythonIO {

    static std::vector<Point> numpyToPoints(py::array_t<double> input);

    static py::array_t<double> pointsToNumpy(std::vector<Point> input);

}

namespace minEnclosingTriangle {

static void advance(unsigned int &index, unsigned int nrOfPoints);

static void advanceBToRightChain(const std::vector<Point> &convexHull,
                                 unsigned int nrOfPoints, unsigned int &b,
                                 unsigned int c);

static bool almostEqual(double number1, double number2);

static double angleOfLineWrtOxAxis(const Point &a, const Point &b);

static bool areEqualPoints(const Point &point1, const Point &point2);

static bool areIdenticalLines(const std::vector<double> &side1Params,
                              const std::vector<double> &side2Params, double sideCExtraParam);

static bool areIdenticalLines(double a1, double b1, double c1, double a2, double b2, double c2);

static bool areIntersectingLines(const std::vector<double> &side1Params,
                                 const std::vector<double> &side2Params,
                                 double sideCExtraParam, Point &intersectionPoint1,
                                 Point &intersectionPoint2);

static bool areOnTheSameSideOfLine(const Point &p1, const Point &p2,
                                   const Point &a, const Point &b);

static double areaOfTriangle(const Point &a, const Point &b, const Point &c);

static double distanceBtwPoints(const Point &a, const Point &b);

static double distanceFromPointToLine(const Point &a, const Point &linePointB,
                                      const Point &linePointC);

static bool findGammaIntersectionPoints(const std::vector<Point> &convexHull, unsigned int nrOfPoints,
                                        unsigned int c, unsigned int convexHullPointIndex,
                                        const Point &side1StartVertex, const Point &side1EndVertex,
                                        const Point &side2StartVertex, const Point &side2EndVertex,
                                        Point &intersectionPoint1, Point &intersectionPoint2);

static void findMinEnclosingTriangle(const std::vector<Point> &convexHull, std::vector<Point> &triangle);

static void findMinimumAreaEnclosingTriangle(const std::vector<Point> &convexHull,
                                             std::vector<Point> &triangle, double &area);

static Point findVertexCOnSideB(const std::vector<Point> &convexHull, unsigned int nrOfPoints,
                                      unsigned int a, unsigned int c,
                                      const Point &sideBStartVertex,
                                      const Point &sideBEndVertex,
                                      const Point &sideCStartVertex,
                                      const Point &sideCEndVertex);

static bool gamma(unsigned int convexHullPointIndex, Point &gammaPoint,
                  const std::vector<Point> &convexHull, unsigned int nrOfPoints,
                  unsigned int a, unsigned int c);

static bool greaterOrEqual(double number1, double number2);

static double height(const Point &convexHullPoint, const std::vector<Point> &convexHull,
                     unsigned int nrOfPoints, unsigned int c);

static double height(unsigned int convexHullPointIndex, const std::vector<Point> &convexHull,
                     unsigned int nrOfPoints, unsigned int c);

static void initialise(std::vector<Point> &triangle, double &area);

static unsigned int intersects(double angleGammaAndPoint, unsigned int convexHullPointIndex,
                               const std::vector<Point> &convexHull, unsigned int nrOfPoints,
                               unsigned int c);

static bool intersectsAbove(const Point &gammaPoint, unsigned int convexHullPointIndex,
                            const std::vector<Point> &convexHull, unsigned int nrOfPoints,
                            unsigned int c);

static unsigned int intersectsAboveOrBelow(unsigned int succPredIndex, unsigned int pointIndex,
                                           const std::vector<Point> &convexHull,
                                           unsigned int nrOfPoints, unsigned int c);

static bool intersectsBelow(const Point &gammaPoint, unsigned int convexHullPointIndex,
                            const std::vector<Point> &convexHull, unsigned int nrOfPoints,
                            unsigned int c);

static bool isAngleBetween(double angle1, double angle2, double angle3);

static bool isAngleBetweenNonReflex(double angle1, double angle2, double angle3);

static bool isFlushAngleBtwPredAndSucc(double &angleFlushEdge, double anglePred, double angleSucc);

static bool isGammaAngleBtw(double &gammaAngle, double angle1, double angle2);

static bool isGammaAngleEqualTo(double &gammaAngle, double angle);

static bool isLocalMinimalTriangle(Point &vertexA, Point &vertexB,
                                   Point &vertexC, const std::vector<Point> &convexHull,
                                   unsigned int nrOfPoints, unsigned int a, unsigned int b,
                                   unsigned int validationFlag, const Point &sideAStartVertex,
                                   const Point &sideAEndVertex, const Point &sideBStartVertex,
                                   const Point &sideBEndVertex, const Point &sideCStartVertex,
                                   const Point &sideCEndVertex);

static bool isNotBTangency(const std::vector<Point> &convexHull,
                           unsigned int nrOfPoints, unsigned int a, unsigned int b,
                           unsigned int c);

static bool isOppositeAngleBetweenNonReflex(double angle1, double angle2, double angle3);

static bool isPointOnLineSegment(const Point &point, const Point &lineSegmentStart,
                                 const Point &lineSegmentEnd);

static bool isValidMinimalTriangle(const Point &vertexA, const Point &vertexB,
                                   const Point &vertexC, const std::vector<Point> &convexHull,
                                   unsigned int nrOfPoints, unsigned int a, unsigned int b,
                                   unsigned int validationFlag, const Point &sideAStartVertex,
                                   const Point &sideAEndVertex, const Point &sideBStartVertex,
                                   const Point &sideBEndVertex, const Point &sideCStartVertex,
                                   const Point &sideCEndVertex);

static bool lessOrEqual(double number1, double number2);

static void lineEquationDeterminedByPoints(const Point &p, const Point &q,
                                           double &a, double &b, double &c);

static std::vector<double> lineEquationParameters(const Point& p, const Point &q);

static bool lineIntersection(const Point &a1, const Point &b1, const Point &a2,
                             const Point &b2, Point &intersection);

static bool lineIntersection(double a1, double b1, double c1, double a2, double b2, double c2,
                             Point &intersection);

static double maximum(double number1, double number2, double number3);

static Point middlePoint(const Point &a, const Point &b);

static bool middlePointOfSideB(Point &middlePointOfSideB, const Point &sideAStartVertex,
                               const Point &sideAEndVertex, const Point &sideBStartVertex,
                               const Point &sideBEndVertex, const Point &sideCStartVertex,
                               const Point &sideCEndVertex);

static void moveAIfLowAndBIfHigh(const std::vector<Point> &convexHull,
                                 unsigned int nrOfPoints, unsigned int &a, unsigned int &b,
                                 unsigned int c);

static double oppositeAngle(double angle);

static unsigned int predecessor(unsigned int index, unsigned int nrOfPoints);

static void returnMinimumAreaEnclosingTriangle(const std::vector<Point> &convexHull,
                                               std::vector<Point> &triangle, double &area);

static void searchForBTangency(const std::vector<Point> &convexHull,
                               unsigned int nrOfPoints, unsigned int a, unsigned int &b,
                               unsigned int c);

static int sign(double number);

static unsigned int successor(unsigned int index, unsigned int nrOfPoints);

static void updateMinimumAreaEnclosingTriangle(std::vector<Point> &triangle, double &area,
                                               const Point &vertexA, const Point &vertexB,
                                               const Point &vertexC);

static void updateSideB(const std::vector<Point> &convexHull,
                        unsigned int nrOfPoints, unsigned int a, unsigned int b,
                        unsigned int c, unsigned int &validationFlag,
                        Point &sideBStartVertex, Point &sideBEndVertex);

static void updateSidesBA(const std::vector<Point> &convexHull,
                          unsigned int nrOfPoints, unsigned int a, unsigned int b,
                          unsigned int c, unsigned int &validationFlag,
                          Point &sideAStartVertex, Point &sideAEndVertex,
                          Point &sideBStartVertex, Point &sideBEndVertex,
                          const Point &sideCStartVertex, const Point &sideCEndVertex);

static void updateSidesCA(const std::vector<Point> &convexHull,
                          unsigned int nrOfPoints, unsigned int a, unsigned int c,
                          Point &sideAStartVertex, Point &sideAEndVertex,
                          Point &sideCStartVertex, Point &sideCEndVertex);

}


///////////////////////////////////// Main functions /////////////////////////////////////

py::array_t<double> computeMinEnclosingTriangle(py::array_t<double> convex_hull) {

    std::vector<Point> convexHull = pythonIO::numpyToPoints(convex_hull);

    std::vector<Point> triangle;
    minEnclosingTriangle::findMinEnclosingTriangle(convexHull, triangle);

    py::array_t<double> pyTriangle = pythonIO::pointsToNumpy(triangle);

    return pyTriangle;
}

PYBIND11_MODULE(pymintriangle, m)
{
    m.doc() = R"pbdoc(
        pymintriangle
        ------------
        .. currentmodule:: pymintriangle
        .. autosummary::
           :toctree: _generate
           compute
    )pbdoc";

    m.def("compute", &computeMinEnclosingTriangle, R"pbdoc(
        Computes the minimal area triangle of set of points making a convex hull.

        Parameters
        ----------
        convexHull : (Mx2) array
            The coordinates of the points making the convex hull.

        Returns
        -------
        triangle : (3x2) array
            The coordinates of the points of the triangle.
    )pbdoc");
}

/////////////////////////////// Helper functions definition //////////////////////////////

namespace pythonIO {

    static std::vector<Point> numpyToPoints(py::array_t<double> input) {
        std::vector<Point> points(input.shape(0));
        Point p;
        auto r = input.unchecked<2>();

        for (ssize_t i = 0; i < r.shape(0); i++)
        {
            p.x = r(i, 0);
            p.y = r(i, 1);
            points[i] = p;
        }

        return points;
    }

    static py::array_t<double> pointsToNumpy(std::vector<Point> input) {
        int numPoints = input.size();
        auto arr = py::array_t<double>({numPoints, 2});
        auto output = arr.mutable_unchecked<2>();
        for (ssize_t i = 0; i < numPoints; i++)
        {
            output(i, 0) = input[i].x;
            output(i, 1) = input[i].y;
        }

        return arr;
    }

}

namespace minEnclosingTriangle {

//! Find the minimum enclosing triangle and its area
/*!
* The overall complexity of the algorithm is theta(n) where "n" represents the number
* of vertices in the convex polygon
*
* @param convexHull     The polygon representing the convex hull of the points
* @param triangle       Minimum area triangle enclosing the given polygon
*/
static void findMinEnclosingTriangle(const std::vector<Point> &convexHull, std::vector<Point> &triangle) {
    double area;

    initialise(triangle, area);

    if (convexHull.size() > 3) {
        findMinimumAreaEnclosingTriangle(convexHull, triangle, area);
    } else {
        returnMinimumAreaEnclosingTriangle(convexHull, triangle, area);
    }

}

//! Initialisation function
/*!
* @param triangle       Minimum area triangle enclosing the given polygon
* @param area           Area of the minimum area enclosing triangle
*/
static void initialise(std::vector<Point> &triangle, double &area) {
    area = std::numeric_limits<double>::max();

    // Clear all points previously stored in the vector
    triangle.clear();
}

//! Find the minimum area enclosing triangle for the given polygon
/*!
* @param convexHull     The polygon representing the convex hull of the points
* @param triangle       Minimum area triangle enclosing the given polygon
* @param area           Area of the minimum area enclosing triangle
*/
static void findMinimumAreaEnclosingTriangle(const std::vector<Point> &convexHull,
                                             std::vector<Point> &triangle, double &area) {
    // Algorithm specific variables

    unsigned int validationFlag;

    Point vertexA, vertexB, vertexC;

    Point sideAStartVertex, sideAEndVertex;
    Point sideBStartVertex, sideBEndVertex;
    Point sideCStartVertex, sideCEndVertex;

    unsigned int a, b, c;
    unsigned int nrOfPoints;

    // Variables initialisation

    nrOfPoints = static_cast<unsigned int>(convexHull.size());

    a = 1;
    b = 2;

    // Main algorithm steps

    for (c = 0; c < nrOfPoints; c++) {
        advanceBToRightChain(convexHull, nrOfPoints, b, c);
        moveAIfLowAndBIfHigh(convexHull, nrOfPoints, a, b, c);
        searchForBTangency(convexHull, nrOfPoints, a ,b, c);

        updateSidesCA(convexHull, nrOfPoints, a, c, sideAStartVertex, sideAEndVertex,
                      sideCStartVertex, sideCEndVertex);

        if (isNotBTangency(convexHull, nrOfPoints, a, b, c)) {
            updateSidesBA(convexHull, nrOfPoints, a, b, c, validationFlag, sideAStartVertex,
                          sideAEndVertex, sideBStartVertex, sideBEndVertex,
                          sideCStartVertex, sideCEndVertex);
        } else {
            updateSideB(convexHull, nrOfPoints, a, b, c, validationFlag,
                        sideBStartVertex,  sideBEndVertex);
        }

        if (isLocalMinimalTriangle(vertexA, vertexB, vertexC, convexHull, nrOfPoints, a, b,
                                   validationFlag, sideAStartVertex, sideAEndVertex,
                                   sideBStartVertex, sideBEndVertex, sideCStartVertex,
                                   sideCEndVertex)) {
            updateMinimumAreaEnclosingTriangle(triangle, area, vertexA, vertexB, vertexC);
        }
    }
}

//! Return the minimum area enclosing (pseudo-)triangle in case the convex polygon has at most three points
/*!
* @param convexHull     The polygon representing the convex hull of the points
* @param triangle   Minimum area triangle enclosing the given polygon
* @param area       Area of the minimum area enclosing triangle
*/
static void returnMinimumAreaEnclosingTriangle(const std::vector<Point> &convexHull,
                                               std::vector<Point> &triangle, double &area) {
    unsigned int nrOfPoints = static_cast<unsigned int>(convexHull.size());

    for (int i = 0; i < 3; i++) {
        triangle.push_back(convexHull[i % nrOfPoints]);
    }

    area = areaOfTriangle(triangle[0], triangle[1], triangle[2]);
}

//! Advance b to the right chain
/*!
* See paper [2] for more details
*
* @param convexHull         The polygon representing the convex hull of the points
* @param nrOfPoints         Number of points defining the convex polygon
* @param b                  Index b
* @param c                  Index c
*/
static void advanceBToRightChain(const std::vector<Point> &convexHull,
                                 unsigned int nrOfPoints, unsigned int &b,
                                 unsigned int c) {
    while (greaterOrEqual(height(successor(b, nrOfPoints), convexHull, nrOfPoints, c),
                          height(b, convexHull, nrOfPoints, c))) {
        advance(b, nrOfPoints);
    }
}

//! Move "a" if it is low and "b" if it is high
/*!
* See paper [2] for more details
*
* @param convexHull         The polygon representing the convex hull of the points
* @param nrOfPoints         Number of points defining the convex polygon
* @param a                  Index a
* @param b                  Index b
* @param c                  Index c
*/
static void moveAIfLowAndBIfHigh(const std::vector<Point> &convexHull,
                                 unsigned int nrOfPoints, unsigned int &a, unsigned int &b,
                                 unsigned int c) {
    Point gammaOfA;

    while(height(b, convexHull, nrOfPoints, c) > height(a, convexHull, nrOfPoints, c)) {
        if ((gamma(a, gammaOfA, convexHull, nrOfPoints, a, c)) && (intersectsBelow(gammaOfA, b, convexHull, nrOfPoints, c))) {
            advance(b, nrOfPoints);
        } else {
            advance(a, nrOfPoints);
        }
    }
}

//! Search for the tangency of side B
/*!
* See paper [2] for more details
*
* @param convexHull         The polygon representing the convex hull of the points
* @param nrOfPoints         Number of points defining the convex polygon
* @param a                  Index a
* @param b                  Index b
* @param c                  Index c
*/
static void searchForBTangency(const std::vector<Point> &convexHull,
                               unsigned int nrOfPoints, unsigned int a, unsigned int &b,
                               unsigned int c) {
    Point gammaOfB;

    while (((gamma(b, gammaOfB, convexHull, nrOfPoints, a, c)) &&
            (intersectsBelow(gammaOfB, b, convexHull, nrOfPoints, c))) &&
           (greaterOrEqual(height(b, convexHull, nrOfPoints, c),
                           height(predecessor(a, nrOfPoints), convexHull, nrOfPoints, c)))
          ) {
        advance(b, nrOfPoints);
    }
}

//! Check if tangency for side B was not obtained
/*!
* See paper [2] for more details
*
* @param convexHull         The polygon representing the convex hull of the points
* @param nrOfPoints         Number of points defining the convex polygon
* @param a                  Index a
* @param b                  Index b
* @param c                  Index c
*/
static bool isNotBTangency(const std::vector<Point> &convexHull,
                           unsigned int nrOfPoints, unsigned int a, unsigned int b,
                           unsigned int c) {
    Point gammaOfB;

    if (((gamma(b, gammaOfB, convexHull, nrOfPoints, a, c)) &&
         (intersectsAbove(gammaOfB, b, convexHull, nrOfPoints, c))) ||
        (height(b, convexHull, nrOfPoints, c) < height(predecessor(a, nrOfPoints), convexHull, nrOfPoints, c))) {
        return true;
    }

    return false;
}

//! Update sides A and C
/*!
* Side C will have as start and end vertices the polygon points "c" and "c-1"
* Side A will have as start and end vertices the polygon points "a" and "a-1"
*
* @param convexHull         The polygon representing the convex hull of the points
* @param nrOfPoints         Number of points defining the convex polygon
* @param a                  Index a
* @param c                  Index c
* @param sideAStartVertex   Start vertex for defining side A
* @param sideAEndVertex     End vertex for defining side A
* @param sideCStartVertex   Start vertex for defining side C
* @param sideCEndVertex     End vertex for defining side C
*/
static void updateSidesCA(const std::vector<Point> &convexHull,
                          unsigned int nrOfPoints, unsigned int a, unsigned int c,
                          Point &sideAStartVertex, Point &sideAEndVertex,
                          Point &sideCStartVertex, Point &sideCEndVertex) {
    sideCStartVertex = convexHull[predecessor(c, nrOfPoints)];
    sideCEndVertex = convexHull[c];

    sideAStartVertex = convexHull[predecessor(a, nrOfPoints)];
    sideAEndVertex = convexHull[a];
}

//! Update sides B and possibly A if tangency for side B was not obtained
/*!
* See paper [2] for more details
*
* @param convexHull         The polygon representing the convex hull of the points
* @param nrOfPoints         Number of points defining the convex polygon
* @param a                  Index a
* @param b                  Index b
* @param c                  Index c
* @param validationFlag     Flag used for validation
* @param sideAStartVertex   Start vertex for defining side A
* @param sideAEndVertex     End vertex for defining side A
* @param sideBStartVertex   Start vertex for defining side B
* @param sideBEndVertex     End vertex for defining side B
* @param sideCStartVertex   Start vertex for defining side C
* @param sideCEndVertex     End vertex for defining side C
*/
static void updateSidesBA(const std::vector<Point> &convexHull,
                          unsigned int nrOfPoints, unsigned int a, unsigned int b,
                          unsigned int c, unsigned int &validationFlag,
                          Point &sideAStartVertex, Point &sideAEndVertex,
                          Point &sideBStartVertex, Point &sideBEndVertex,
                          const Point &sideCStartVertex, const Point &sideCEndVertex) {
    // Side B is flush with edge [b, b-1]
    sideBStartVertex = convexHull[predecessor(b, nrOfPoints)];
    sideBEndVertex = convexHull[b];

    // Find middle point of side B
    Point sideBMiddlePoint;

    if ((middlePointOfSideB(sideBMiddlePoint, sideAStartVertex, sideAEndVertex, sideBStartVertex,
                            sideBEndVertex, sideCStartVertex, sideCEndVertex)) &&
        (height(sideBMiddlePoint, convexHull, nrOfPoints, c) <
         height(predecessor(a, nrOfPoints), convexHull, nrOfPoints, c))) {
        sideAStartVertex = convexHull[predecessor(a, nrOfPoints)];
        sideAEndVertex = findVertexCOnSideB(convexHull, nrOfPoints, a, c,
                                            sideBStartVertex, sideBEndVertex,
                                            sideCStartVertex, sideCEndVertex);

        validationFlag = VALIDATION_SIDE_A_TANGENT;
    } else {
        validationFlag = VALIDATION_SIDES_FLUSH;
    }
}

//! Set side B if tangency for side B was obtained
/*!
* See paper [2] for more details
*
* @param convexHull         The polygon representing the convex hull of the points
* @param nrOfPoints         Number of points defining the convex polygon
* @param a                  Index a
* @param b                  Index b
* @param c                  Index c
* @param validationFlag     Flag used for validation
* @param sideBStartVertex   Start vertex for defining side B
* @param sideBEndVertex     End vertex for defining side B
*/
static void updateSideB(const std::vector<Point> &convexHull,
                        unsigned int nrOfPoints, unsigned int a, unsigned int b,
                        unsigned int c, unsigned int &validationFlag,
                        Point &sideBStartVertex, Point &sideBEndVertex) {
    if (!gamma(b, sideBStartVertex, convexHull, nrOfPoints, a, c)) {
        throw std::invalid_argument(ERR_SIDE_B_GAMMA);
    }

    sideBEndVertex = convexHull[b];

    validationFlag = VALIDATION_SIDE_B_TANGENT;
}

//! Update the triangle vertices after all sides were set and check if a local minimal triangle was found or not
/*!
* See paper [2] for more details
*
* @param vertexA            Vertex A of the enclosing triangle
* @param vertexB            Vertex B of the enclosing triangle
* @param vertexC            Vertex C of the enclosing triangle
* @param convexHull         The polygon representing the convex hull of the points
* @param nrOfPoints         Number of points defining the convex polygon
* @param a                  Index a
* @param b                  Index b
* @param validationFlag     Flag used for validation
* @param sideAStartVertex   Start vertex for defining side A
* @param sideAEndVertex     End vertex for defining side A
* @param sideBStartVertex   Start vertex for defining side B
* @param sideBEndVertex     End vertex for defining side B
* @param sideCStartVertex   Start vertex for defining side C
* @param sideCEndVertex     End vertex for defining side C
*/
static bool isLocalMinimalTriangle(Point &vertexA, Point &vertexB,
                                   Point &vertexC, const std::vector<Point> &convexHull,
                                   unsigned int nrOfPoints, unsigned int a, unsigned int b,
                                   unsigned int validationFlag, const Point &sideAStartVertex,
                                   const Point &sideAEndVertex, const Point &sideBStartVertex,
                                   const Point &sideBEndVertex, const Point &sideCStartVertex,
                                   const Point &sideCEndVertex) {
    if ((!lineIntersection(sideAStartVertex, sideAEndVertex,
                           sideBStartVertex, sideBEndVertex, vertexC)) ||
        (!lineIntersection(sideAStartVertex, sideAEndVertex,
                           sideCStartVertex, sideCEndVertex, vertexB)) ||
        (!lineIntersection(sideBStartVertex, sideBEndVertex,
                           sideCStartVertex, sideCEndVertex, vertexA))) {
        return false;
    }

    return isValidMinimalTriangle(vertexA, vertexB, vertexC, convexHull, nrOfPoints, a, b,
                                  validationFlag, sideAStartVertex, sideAEndVertex,
                                  sideBStartVertex, sideBEndVertex, sideCStartVertex,
                                  sideCEndVertex);
}

//! Check if the found minimal triangle is valid
/*!
* This means that all midpoints of the triangle should touch the polygon
*
* See paper [2] for more details
*
* @param vertexA            Vertex A of the enclosing triangle
* @param vertexB            Vertex B of the enclosing triangle
* @param vertexC            Vertex C of the enclosing triangle
* @param convexHull         The polygon representing the convex hull of the points
* @param nrOfPoints         Number of points defining the convex polygon
* @param a                  Index a
* @param b                  Index b
* @param validationFlag     Flag used for validation
* @param sideAStartVertex   Start vertex for defining side A
* @param sideAEndVertex     End vertex for defining side A
* @param sideBStartVertex   Start vertex for defining side B
* @param sideBEndVertex     End vertex for defining side B
* @param sideCStartVertex   Start vertex for defining side C
* @param sideCEndVertex     End vertex for defining side C
*/
static bool isValidMinimalTriangle(const Point &vertexA, const Point &vertexB,
                                   const Point &vertexC, const std::vector<Point> &convexHull,
                                   unsigned int nrOfPoints, unsigned int a, unsigned int b,
                                   unsigned int validationFlag, const Point &sideAStartVertex,
                                   const Point &sideAEndVertex, const Point &sideBStartVertex,
                                   const Point &sideBEndVertex, const Point &sideCStartVertex,
                                   const Point &sideCEndVertex) {
    Point midpointSideA = middlePoint(vertexB, vertexC);
    Point midpointSideB = middlePoint(vertexA, vertexC);
    Point midpointSideC = middlePoint(vertexA, vertexB);

    bool sideAValid = (validationFlag == VALIDATION_SIDE_A_TANGENT)
                        ? (areEqualPoints(midpointSideA, convexHull[predecessor(a, nrOfPoints)]))
                        : (isPointOnLineSegment(midpointSideA, sideAStartVertex, sideAEndVertex));

    bool sideBValid = (validationFlag == VALIDATION_SIDE_B_TANGENT)
                          ? (areEqualPoints(midpointSideB, convexHull[b]))
                          : (isPointOnLineSegment(midpointSideB, sideBStartVertex, sideBEndVertex));

    bool sideCValid = (validationFlag == VALIDATION_SIDES_FLUSH) || isPointOnLineSegment(midpointSideC, sideCStartVertex, sideCEndVertex);

    return (sideAValid && sideBValid && sideCValid);
}

//! Update the current minimum area enclosing triangle if the newly obtained one has a smaller area
/*!
* @param triangle   Minimum area triangle enclosing the given polygon
* @param area       Area of the minimum area triangle enclosing the given polygon
* @param vertexA    Vertex A of the enclosing triangle
* @param vertexB    Vertex B of the enclosing triangle
* @param vertexC    Vertex C of the enclosing triangle
*/
static void updateMinimumAreaEnclosingTriangle(std::vector<Point> &triangle, double &area,
                                               const Point &vertexA, const Point &vertexB,
                                               const Point &vertexC) {
    double triangleArea = areaOfTriangle(vertexA, vertexB, vertexC);

    if (triangleArea < area) {
        triangle.clear();

        triangle.push_back(vertexA);
        triangle.push_back(vertexB);
        triangle.push_back(vertexC);

        area = triangleArea;
    }
}

//! Return the middle point of side B
/*!
* @param middlePointOfSideB Middle point of side B
* @param sideAStartVertex   Start vertex for defining side A
* @param sideAEndVertex     End vertex for defining side A
* @param sideBStartVertex   Start vertex for defining side B
* @param sideBEndVertex     End vertex for defining side B
* @param sideCStartVertex   Start vertex for defining side C
* @param sideCEndVertex     End vertex for defining side C
*/
static bool middlePointOfSideB(Point &middlePointOfSideB, const Point &sideAStartVertex,
                               const Point &sideAEndVertex, const Point &sideBStartVertex,
                               const Point &sideBEndVertex, const Point &sideCStartVertex,
                               const Point &sideCEndVertex) {
    Point vertexA, vertexC;

    if ((!lineIntersection(sideBStartVertex, sideBEndVertex, sideCStartVertex, sideCEndVertex, vertexA)) ||
        (!lineIntersection(sideBStartVertex, sideBEndVertex, sideAStartVertex, sideAEndVertex, vertexC))) {
        return false;
    }

    middlePointOfSideB = middlePoint(vertexA, vertexC);

    return true;
}

//! Check if the line intersects below
/*!
* Check if the line determined by gammaPoint and polygon[polygonPointIndex] intersects
* the polygon below the point polygon[polygonPointIndex]
*
* @param gammaPoint         Gamma(p)
* @param polygonPointIndex  Index of the polygon point which is considered when determining the line
* @param convexHull         The polygon representing the convex hull of the points
* @param nrOfPoints         Number of points defining the convex polygon
* @param c                  Index c
*/
static bool intersectsBelow(const Point &gammaPoint, unsigned int convexHullPointIndex,
                            const std::vector<Point> &convexHull, unsigned int nrOfPoints,
                            unsigned int c) {
    double angleOfGammaAndPoint = angleOfLineWrtOxAxis(convexHull[convexHullPointIndex], gammaPoint);

    return (intersects(angleOfGammaAndPoint, convexHullPointIndex, convexHull, nrOfPoints, c) == INTERSECTS_BELOW);
}

//! Check if the line intersects above
/*!
* Check if the line determined by gammaPoint and polygon[polygonPointIndex] intersects
* the polygon above the point polygon[polygonPointIndex]
*
* @param gammaPoint         Gamma(p)
* @param polygonPointIndex  Index of the polygon point which is considered when determining the line
* @param convexHull         The polygon representing the convex hull of the points
* @param nrOfPoints         Number of points defining the convex polygon
* @param c                  Index c
*/
static bool intersectsAbove(const Point &gammaPoint, unsigned int convexHullPointIndex,
                            const std::vector<Point> &convexHull, unsigned int nrOfPoints,
                            unsigned int c) {
    double angleOfGammaAndPoint = angleOfLineWrtOxAxis(gammaPoint, convexHull[convexHullPointIndex]);

    return (intersects(angleOfGammaAndPoint, convexHullPointIndex, convexHull, nrOfPoints, c) == INTERSECTS_ABOVE);
}

//! Check if/where the line determined by gammaPoint and polygon[polygonPointIndex] intersects the polygon
/*!
* @param angleGammaAndPoint     Angle determined by gammaPoint and polygon[polygonPointIndex] wrt Ox axis
* @param polygonPointIndex      Index of the polygon point which is considered when determining the line
* @param polygon                The polygon representing the convex hull of the points
* @param nrOfPoints             Number of points defining the convex polygon
* @param c                      Index c
*/
static unsigned int intersects(double angleGammaAndPoint, unsigned int convexHullPointIndex,
                               const std::vector<Point> &convexHull, unsigned int nrOfPoints,
                               unsigned int c) {
    double anglePointPredecessor = angleOfLineWrtOxAxis(convexHull[predecessor(convexHullPointIndex, nrOfPoints)],
                                                        convexHull[convexHullPointIndex]);
    double anglePointSuccessor   = angleOfLineWrtOxAxis(convexHull[successor(convexHullPointIndex, nrOfPoints)],
                                                        convexHull[convexHullPointIndex]);
    double angleFlushEdge        = angleOfLineWrtOxAxis(convexHull[predecessor(c, nrOfPoints)],
                                                        convexHull[c]);

    if (isFlushAngleBtwPredAndSucc(angleFlushEdge, anglePointPredecessor, anglePointSuccessor)) {
        if ((isGammaAngleBtw(angleGammaAndPoint, anglePointPredecessor, angleFlushEdge)) ||
            (almostEqual(angleGammaAndPoint, anglePointPredecessor))) {
            return intersectsAboveOrBelow(predecessor(convexHullPointIndex, nrOfPoints),
                                          convexHullPointIndex, convexHull, nrOfPoints, c);
        } else if ((isGammaAngleBtw(angleGammaAndPoint, anglePointSuccessor, angleFlushEdge)) ||
                  (almostEqual(angleGammaAndPoint, anglePointSuccessor))) {
            return intersectsAboveOrBelow(successor(convexHullPointIndex, nrOfPoints),
                                          convexHullPointIndex, convexHull, nrOfPoints, c);
        }
    } else {
        if (
            (isGammaAngleBtw(angleGammaAndPoint, anglePointPredecessor, anglePointSuccessor)) ||
            (
                (isGammaAngleEqualTo(angleGammaAndPoint, anglePointPredecessor)) &&
                (!isGammaAngleEqualTo(angleGammaAndPoint, angleFlushEdge))
            ) ||
            (
                (isGammaAngleEqualTo(angleGammaAndPoint, anglePointSuccessor)) &&
                (!isGammaAngleEqualTo(angleGammaAndPoint, angleFlushEdge))
            )
           ) {
            return INTERSECTS_BELOW;
        }
    }

    return INTERSECTS_CRITICAL;
}

//! If (gamma(x) x) intersects P between successorOrPredecessorIndex and pointIntex is it above/below?
/*!
* @param succPredIndex  Index of the successor or predecessor
* @param pointIndex     Index of the point x in the polygon
* @param polygon        The polygon representing the convex hull of the points
* @param nrOfPoints     Number of points defining the convex polygon
* @param c              Index c
*/
static unsigned int intersectsAboveOrBelow(unsigned int succPredIndex, unsigned int pointIndex,
                                           const std::vector<Point> &convexHull,
                                           unsigned int nrOfPoints, unsigned int c) {
    if (height(succPredIndex, convexHull, nrOfPoints, c) > height(pointIndex, convexHull, nrOfPoints, c)) {
        return INTERSECTS_ABOVE;
    } else {
        return INTERSECTS_BELOW;
    }
}

//! Find gamma for a given point "p" specified by its index
/*!
* The function returns true if gamma exists i.e. if lines (a a-1) and (x y) intersect
* and false otherwise. In case the two lines intersect in point intersectionPoint, gamma is computed.
*
* Considering that line (x y) is a line parallel to (c c-1) and that the distance between the lines is equal
* to 2 * height(p), we can have two possible (x y) lines.
*
* Therefore, we will compute two intersection points between the lines (x y) and (a a-1) and take the
* point which is on the same side of line (c c-1) as the polygon.
*
* See paper [2] and formula for distance from point to a line for more details
*
* @param polygonPointIndex Index of the polygon point
* @param gammaPoint        Point gamma(polygon[polygonPointIndex])
* @param polygon           The polygon representing the convex hull of the points
* @param nrOfPoints        Number of points defining the convex polygon
* @param a                 Index a
* @param c                 Index c
*/
static bool gamma(unsigned int convexHullPointIndex, Point &gammaPoint,
                  const std::vector<Point> &convexHull, unsigned int nrOfPoints,
                  unsigned int a, unsigned int c) {
    Point intersectionPoint1, intersectionPoint2;

    // Get intersection points if they exist
    if (!findGammaIntersectionPoints(convexHull, nrOfPoints, c, convexHullPointIndex,
                                     convexHull[a], convexHull[predecessor(a, nrOfPoints)],
                                     convexHull[c], convexHull[predecessor(c, nrOfPoints)],
                                     intersectionPoint1, intersectionPoint2)) {
        return false;
    }

    // Select the point which is on the same side of line C as the convexHull
    if (areOnTheSameSideOfLine(intersectionPoint1, convexHull[successor(c, nrOfPoints)],
                               convexHull[c], convexHull[predecessor(c, nrOfPoints)])) {
        gammaPoint = intersectionPoint1;
    } else {
        gammaPoint = intersectionPoint2;
    }

    return true;
}

//! Find vertex C which lies on side B at a distance = 2 * height(a-1) from side C
/*!
* Considering that line (x y) is a line parallel to (c c-1) and that the distance between the lines is equal
* to 2 * height(a-1), we can have two possible (x y) lines.
*
* Therefore, we will compute two intersection points between the lines (x y) and (b b-1) and take the
* point which is on the same side of line (c c-1) as the polygon.
*
* See paper [2] and formula for distance from point to a line for more details
*
* @param convexHull         The polygon representing the convex hull of the points
* @param nrOfPoints         Number of points defining the convex polygon
* @param a                  Index a
* @param c                  Index c
* @param sideBStartVertex   Start vertex for defining side B
* @param sideBEndVertex     End vertex for defining side B
* @param sideCStartVertex   Start vertex for defining side C
* @param sideCEndVertex     End vertex for defining side C
*/
static Point findVertexCOnSideB(const std::vector<Point> &convexHull, unsigned int nrOfPoints,
                                      unsigned int a, unsigned int c,
                                      const Point &sideBStartVertex,
                                      const Point &sideBEndVertex,
                                      const Point &sideCStartVertex,
                                      const Point &sideCEndVertex) {
    Point intersectionPoint1, intersectionPoint2;

    // Get intersection points if they exist
    if (!findGammaIntersectionPoints(convexHull, nrOfPoints, c, predecessor(a, nrOfPoints),
                                     sideBStartVertex, sideBEndVertex,
                                     sideCStartVertex, sideCEndVertex,
                                     intersectionPoint1, intersectionPoint2)) {
        throw std::invalid_argument(ERR_VERTEX_C_ON_SIDE_B);
    }

    // Select the point which is on the same side of line C as the convexHull
    if (areOnTheSameSideOfLine(intersectionPoint1, convexHull[successor(c, nrOfPoints)],
                               convexHull[c], convexHull[predecessor(c, nrOfPoints)])) {
        return intersectionPoint1;
    } else {
        return intersectionPoint2;
    }
}

//! Find the intersection points to compute gamma(point)
/*!
* @param polygon                The polygon representing the convex hull of the points
* @param nrOfPoints             Number of points defining the convex polygon
* @param c                      Index c
* @param polygonPointIndex      Index of the polygon point for which the distance is known
* @param side1StartVertex       Start vertex for side 1
* @param side1EndVertex         End vertex for side 1
* @param side2StartVertex       Start vertex for side 2
* @param side2EndVertex         End vertex for side 2
* @param intersectionPoint1     First intersection point between one pair of lines
* @param intersectionPoint2     Second intersection point between other pair of lines
*/
static bool findGammaIntersectionPoints(const std::vector<Point> &convexHull, unsigned int nrOfPoints,
                                        unsigned int c, unsigned int convexHullPointIndex,
                                        const Point &side1StartVertex, const Point &side1EndVertex,
                                        const Point &side2StartVertex, const Point &side2EndVertex,
                                        Point &intersectionPoint1, Point &intersectionPoint2) {
    std::vector<double> side1Params = lineEquationParameters(side1StartVertex, side1EndVertex);
    std::vector<double> side2Params = lineEquationParameters(side2StartVertex, side2EndVertex);

    // Compute side C extra parameter using the formula for distance from a point to a line
    double convexHullPointHeight = height(convexHullPointIndex, convexHull, nrOfPoints, c);
    double distFormulaDenom = sqrt((side2Params[0] * side2Params[0]) + (side2Params[1] * side2Params[1]));
    double sideCExtraParam = 2 * convexHullPointHeight * distFormulaDenom;

    // Get intersection points if they exist or if lines are identical
    if (areIntersectingLines(side1Params, side2Params, sideCExtraParam, intersectionPoint1, intersectionPoint2)) {
        return true;
    } else if (areIdenticalLines(side1Params, side2Params, sideCExtraParam)) {
        intersectionPoint1 = side1StartVertex;
        intersectionPoint2 = side1EndVertex;
        return true;
    }

    return false;
}

//! Check if the given lines are identical or not
/*!
* The lines are specified as:
*      ax + by + c = 0
*  OR
*      ax + by + c (+/-) sideCExtraParam = 0
*
* @param side1Params       Vector containing the values of a, b and c for side 1
* @param side2Params       Vector containing the values of a, b and c for side 2
* @param sideCExtraParam   Extra parameter for the flush edge C
*/
static bool areIdenticalLines(const std::vector<double> &side1Params,
                              const std::vector<double> &side2Params, double sideCExtraParam) {
    return (
        (areIdenticalLines(side1Params[0], side1Params[1], -(side1Params[2]),
                           side2Params[0], side2Params[1], -(side2Params[2]) - sideCExtraParam)) ||
        (areIdenticalLines(side1Params[0], side1Params[1], -(side1Params[2]),
                           side2Params[0], side2Params[1], -(side2Params[2]) + sideCExtraParam))
    );
}

//! Check if the given lines intersect or not. If the lines intersect find their intersection points.
/*!
* The lines are specified as:
*      ax + by + c = 0
*  OR
*      ax + by + c (+/-) sideCExtraParam = 0
*
* @param side1Params           Vector containing the values of a, b and c for side 1
* @param side2Params           Vector containing the values of a, b and c for side 2
* @param sideCExtraParam       Extra parameter for the flush edge C
* @param intersectionPoint1    The first intersection point, if it exists
* @param intersectionPoint2    The second intersection point, if it exists
*/
static bool areIntersectingLines(const std::vector<double> &side1Params,
                                 const std::vector<double> &side2Params,
                                 double sideCExtraParam, Point &intersectionPoint1,
                                 Point &intersectionPoint2) {
    return (
        (lineIntersection(side1Params[0], side1Params[1], -(side1Params[2]),
                          side2Params[0], side2Params[1], -(side2Params[2]) - sideCExtraParam,
                          intersectionPoint1)) &&
        (lineIntersection(side1Params[0], side1Params[1], -(side1Params[2]),
                          side2Params[0], side2Params[1], -(side2Params[2]) + sideCExtraParam,
                          intersectionPoint2))
    );
}

//! Get the line equation parameters "a", "b" and "c" for the line determined by points "p" and "q"
/*!
* The equation of the line is considered in the general form:
* ax + by + c = 0
*
* @param p One point for defining the equation of the line
* @param q Second point for defining the equation of the line
*/
static std::vector<double> lineEquationParameters(const Point& p, const Point &q) {
    std::vector<double> lineEquationParameters;
    double a, b, c;

    lineEquationDeterminedByPoints(p, q, a, b, c);

    lineEquationParameters.push_back(a);
    lineEquationParameters.push_back(b);
    lineEquationParameters.push_back(c);

    return lineEquationParameters;
}

//! Compute the height of the point
/*!
* See paper [2] for more details
*
* @param polygonPoint       Polygon point
* @param convexHull         The polygon representing the convex hull of the points
* @param nrOfPoints         Number of points defining the convex polygon
* @param c                  Index c
*/
static double height(const Point &convexHullPoint, const std::vector<Point> &convexHull,
                     unsigned int nrOfPoints, unsigned int c) {
    Point pointC = convexHull[c];
    Point pointCPredecessor = convexHull[predecessor(c, nrOfPoints)];

    return distanceFromPointToLine(convexHullPoint, pointC, pointCPredecessor);
}

//! Compute the height of the point specified by the given index
/*!
* See paper [2] for more details
*
* @param polygonPointIndex  Index of the polygon point
* @param convexHull         The polygon representing the convex hull of the points
* @param nrOfPoints         Number of points defining the convex polygon
* @param c                  Index c
*/
static double height(unsigned int convexHullPointIndex, const std::vector<Point> &convexHull,
                     unsigned int nrOfPoints, unsigned int c) {
    Point pointC = convexHull[c];
    Point pointCPredecessor = convexHull[predecessor(c, nrOfPoints)];

    Point convexHullPoint = convexHull[convexHullPointIndex];

    return distanceFromPointToLine(convexHullPoint, pointC, pointCPredecessor);
}

//! Advance the given index with one position
/*!
* @param index          Index of the point
* @param nrOfPoints     Number of points defining the convex polygon
*/
static void advance(unsigned int &index, unsigned int nrOfPoints) {
    index = successor(index, nrOfPoints);
}

//! Return the successor of the provided point index
/*!
* The successor of the last polygon point is the first polygon point
* (circular referencing)
*
* @param index          Index of the point
* @param nrOfPoints     Number of points defining the convex polygon
*/
static unsigned int successor(unsigned int index, unsigned int nrOfPoints) {
    return ((index + 1) % nrOfPoints);
}

//! Return the predecessor of the provided point index
/*!
* The predecessor of the first polygon point is the last polygon point
* (circular referencing)
*
* @param index          Index of the point
* @param nrOfPoints     Number of points defining the convex polygon
*/
static unsigned int predecessor(unsigned int index, unsigned int nrOfPoints) {
    return (index == 0) ? (nrOfPoints - 1)
                        : (index - 1);
}

//! Check if the flush edge angle/opposite angle lie between the predecessor and successor angle
/*!
* Check if the angle of the flush edge or its opposite angle lie between the angle of
* the predecessor and successor
*
* @param angleFlushEdge    Angle of the flush edge
* @param anglePred         Angle of the predecessor
* @param angleSucc         Angle of the successor
*/
static bool isFlushAngleBtwPredAndSucc(double &angleFlushEdge, double anglePred, double angleSucc) {
    if (isAngleBetweenNonReflex(angleFlushEdge, anglePred, angleSucc)) {
        return true;
    } else if (isOppositeAngleBetweenNonReflex(angleFlushEdge, anglePred, angleSucc)) {
        angleFlushEdge = oppositeAngle(angleFlushEdge);

        return true;
    }

    return false;
}

//! Check if the angle of the line (gamma(p) p) or its opposite angle is equal to the given angle
/*!
* @param gammaAngle    Angle of the line (gamma(p) p)
* @param angle         Angle to compare against
*/
static bool isGammaAngleEqualTo(double &gammaAngle, double angle) {
    return (almostEqual(gammaAngle, angle));
}

//! Check if the angle of the line (gamma(p) p) or its opposite angle lie between angle1 and angle2
/*!
* @param gammaAngle    Angle of the line (gamma(p) p)
* @param angle1        One of the boundary angles
* @param angle2        Another boundary angle
*/
static bool isGammaAngleBtw(double &gammaAngle, double angle1, double angle2) {
    return (isAngleBetweenNonReflex(gammaAngle, angle1, angle2));
}

//! Get the angle of the line measured from the Ox axis in counterclockwise direction
/*!
* The line is specified by points "a" and "b". The value of the angle is expressed in degrees.
*
* @param a Point a
* @param b Point b
*/
static double angleOfLineWrtOxAxis(const Point &a, const Point &b) {
    double y = b.y - a.y;
    double x = b.x - a.x;

    double angle = (std::atan2(y, x) * 180 / M_PI);

    return (angle < 0) ? (angle + 360)
                       : angle;
}

//! Check if angle1 lies between non reflex angle determined by angles 2 and 3
/*!
* @param angle1 The angle which lies between angle2 and angle3 or not
* @param angle2 One of the boundary angles
* @param angle3 The other boundary angle
*/
static bool isAngleBetweenNonReflex(double angle1, double angle2, double angle3) {
    if (std::abs(angle2 - angle3) > 180) {
        if (angle2 > angle3) {
            return (((angle2 < angle1) && (lessOrEqual(angle1, 360))) ||
                    ((lessOrEqual(0, angle1)) && (angle1 < angle3)));
        } else {
            return (((angle3 < angle1) && (lessOrEqual(angle1, 360))) ||
                    ((lessOrEqual(0, angle1)) && (angle1 < angle2)));
        }
    } else {
        return isAngleBetween(angle1, angle2, angle3);
    }
}

//! Check if the opposite of angle1, ((angle1 + 180) % 360), lies between non reflex angle determined by angles 2 and 3
/*!
* @param angle1 The angle which lies between angle2 and angle3 or not
* @param angle2 One of the boundary angles
* @param angle3 The other boundary angle
*/
static bool isOppositeAngleBetweenNonReflex(double angle1, double angle2, double angle3) {
    double angle1Opposite = oppositeAngle(angle1);

    return (isAngleBetweenNonReflex(angle1Opposite, angle2, angle3));
}

//! Check if angle1 lies between angles 2 and 3
/*!
* @param angle1 The angle which lies between angle2 and angle3 or not
* @param angle2 One of the boundary angles
* @param angle3 The other boundary angle
*/
static bool isAngleBetween(double angle1, double angle2, double angle3) {
    if ((((int)(angle2 - angle3)) % 180) > 0) {
        return ((angle3 < angle1) && (angle1 < angle2));
    } else {
        return ((angle2 < angle1) && (angle1 < angle3));
    }
}

//! Return the angle opposite to the given angle
/*!
* if (angle < 180) then
*      return (angle + 180);
* else
*      return (angle - 180);
* endif
*
* @param angle Angle
*/
static double oppositeAngle(double angle) {
    return (angle > 180) ? (angle - 180)
                         : (angle + 180);
}

//! Compute the distance from a point "a" to a line specified by two points "B" and "C"
/*!
* Formula used:
*
*     |(x_c - x_b) * (y_b - y_a) - (x_b - x_a) * (y_c - y_b)|
* d = -------------------------------------------------------
*            sqrt(((x_c - x_b)^2) + ((y_c - y_b)^2))
*
* Reference: http://mathworld.wolfram.com/Point-LineDistance2-Dimensional.html
* (Last access: 15.09.2013)
*
* @param a             Point from which the distance is measures
* @param linePointB    One of the points determining the line
* @param linePointC    One of the points determining the line
*/
static double distanceFromPointToLine(const Point &a, const Point &linePointB,
                                      const Point &linePointC) {
    double term1 = linePointC.x - linePointB.x;
    double term2 = linePointB.y - a.y;
    double term3 = linePointB.x - a.x;
    double term4 = linePointC.y - linePointB.y;

    double nominator = std::abs((term1 * term2) - (term3 * term4));
    double denominator = std::sqrt((term1 * term1) + (term4 * term4));

    return (denominator != 0) ? (nominator / denominator)
                              : 0;
}

//! Compute the distance between two points
/*! Compute the Euclidean distance between two points
*
* @param a Point a
* @param b Point b
*/
static double distanceBtwPoints(const Point &a, const Point &b) {
    double xDiff = a.x - b.x;
    double yDiff = a.y - b.y;

    return std::sqrt((xDiff * xDiff) + (yDiff * yDiff));
}

//! Compute the area of a triangle defined by three points
/*!
* The area is computed using the determinant method.
* An example is depicted at http://demonstrations.wolfram.com/TheAreaOfATriangleUsingADeterminant/
* (Last access: 15.09.2013)
*
* @param a Point a
* @param b Point b
* @param c Point c
*/
static double areaOfTriangle(const Point &a, const Point &b, const Point &c) {
    double posTerm = (a.x * b.y) + (a.y * c.x) + (b.x * c.y);
    double negTerm = (b.y * c.x) + (a.x * c.y) + (a.y * b.x);

    double determinant = posTerm - negTerm;

    return std::abs(determinant) / 2;
}

//! Get the point in the middle of the segment determined by points "a" and "b"
/*!
* @param a Point a
* @param b Point b
*/
static Point middlePoint(const Point &a, const Point &b) {
    double middleX = (a.x + b.x) / 2;
    double middleY = (a.y + b.y) / 2;

    Point p;
    p.x = middleX;
    p.y = middleY;

    return p;
}

//! Determine the intersection point of two lines, if this point exists
/*! Two lines intersect if they are not parallel (Parallel lines intersect at
* +/- infinity, but we do not consider this case here).
*
* The lines are specified in the following form:
*      A1x + B1x = C1
*      A2x + B2x = C2
*
* If det (= A1*B2 - A2*B1) == 0, then lines are parallel
*                                else they intersect
*
* If they intersect, then let us denote the intersection point with P(x, y) where:
*      x = (C1*B2 - C2*B1) / (det)
*      y = (C2*A1 - C1*A2) / (det)
*
* @param a1 A1
* @param b1 B1
* @param c1 C1
* @param a2 A2
* @param b2 B2
* @param c2 C2
* @param intersection The intersection point, if this point exists
*/
static bool lineIntersection(double a1, double b1, double c1, double a2, double b2, double c2,
                             Point &intersection) {
    double det = (a1 * b2) - (a2 * b1);

    if (!(almostEqual(det, 0))) {
        intersection.x = ((c1 * b2) - (c2 * b1)) / (det);
        intersection.y = ((c2 * a1) - (c1 * a2)) / (det);

        return true;
    }

    return false;
}

//! Determine the intersection point of two lines, if this point exists
/*! Two lines intersect if they are not parallel (Parallel lines intersect at
* +/- infinity, but we do not consider this case here).
*
* The lines are specified by a pair of points each. If they intersect, then
* the function returns true, else it returns false.
*
* Lines can be specified in the following form:
*      A1x + B1x = C1
*      A2x + B2x = C2
*
* If det (= A1*B2 - A2*B1) == 0, then lines are parallel
*                                else they intersect
*
* If they intersect, then let us denote the intersection point with P(x, y) where:
*      x = (C1*B2 - C2*B1) / (det)
*      y = (C2*A1 - C1*A2) / (det)
*
* @param a1 First point for determining the first line
* @param b1 Second point for determining the first line
* @param a2 First point for determining the second line
* @param b2 Second point for determining the second line
* @param intersection The intersection point, if this point exists
*/
static bool lineIntersection(const Point &a1, const Point &b1, const Point &a2,
                             const Point &b2, Point &intersection) {
    double A1 = b1.y - a1.y;
    double B1 = a1.x - b1.x;
    double C1 = (a1.x * A1) + (a1.y * B1);

    double A2 = b2.y - a2.y;
    double B2 = a2.x - b2.x;
    double C2 = (a2.x * A2) + (a2.y * B2);

    double det = (A1 * B2) - (A2 * B1);

    if (!almostEqual(det, 0)) {
        intersection.x = ((C1 * B2) - (C2 * B1)) / (det);
        intersection.y = ((C2 * A1) - (C1 * A2)) / (det);

        return true;
    }

    return false;
}

//! Get the values of "a", "b" and "c" of the line equation ax + by + c = 0 knowing that point "p" and "q" are on the line
/*!
* a = q.y - p.y
* b = p.x - q.x
* c = - (p.x * a) - (p.y * b)
*
* @param p Point p
* @param q Point q
* @param a Parameter "a" from the line equation
* @param b Parameter "b" from the line equation
* @param c Parameter "c" from the line equation
*/
static void lineEquationDeterminedByPoints(const Point &p, const Point &q,
                                           double &a, double &b, double &c) {
    assert(areEqualPoints(p, q) == false);

    a = q.y - p.y;
    b = p.x - q.x;
    c = ((-p.y) * b) - (p.x * a);
}

//! Check if p1 and p2 are on the same side of the line determined by points a and b
/*!
* @param p1    Point p1
* @param p2    Point p2
* @param a     First point for determining line
* @param b     Second point for determining line
*/
static bool areOnTheSameSideOfLine(const Point &p1, const Point &p2,
                                   const Point &a, const Point &b) {
    double a1, b1, c1;

    lineEquationDeterminedByPoints(a, b, a1, b1, c1);

    double p1OnLine = (a1 * p1.x) + (b1 * p1.y) + c1;
    double p2OnLine = (a1 * p2.x) + (b1 * p2.y) + c1;

    return (sign(p1OnLine) == sign(p2OnLine));
}

//! Check if one point lies between two other points
/*!
* @param point             Point lying possibly outside the line segment
* @param lineSegmentStart  First point determining the line segment
* @param lineSegmentEnd    Second point determining the line segment
*/
static bool isPointOnLineSegment(const Point &point, const Point &lineSegmentStart,
                                 const Point &lineSegmentEnd) {
    double d1 = distanceBtwPoints(point, lineSegmentStart);
    double d2 = distanceBtwPoints(point, lineSegmentEnd);
    double lineSegmentLength = distanceBtwPoints(lineSegmentStart, lineSegmentEnd);

    return (almostEqual(d1 + d2, lineSegmentLength));
}

//! Check if two lines are identical
/*!
* Lines are be specified in the following form:
*      A1x + B1x = C1
*      A2x + B2x = C2
*
* If (A1/A2) == (B1/B2) == (C1/C2), then the lines are identical
*                                   else they are not
*
* @param a1 A1
* @param b1 B1
* @param c1 C1
* @param a2 A2
* @param b2 B2
* @param c2 C2
*/
static bool areIdenticalLines(double a1, double b1, double c1, double a2, double b2, double c2) {
    double a1B2 = a1 * b2;
    double a2B1 = a2 * b1;
    double a1C2 = a1 * c2;
    double a2C1 = a2 * c1;
    double b1C2 = b1 * c2;
    double b2C1 = b2 * c1;

    return ((almostEqual(a1B2, a2B1)) && (almostEqual(b1C2, b2C1)) && (almostEqual(a1C2, a2C1)));
}

//! Check if points point1 and point2 are equal or not
/*!
* @param point1 One point
* @param point2 The other point
*/
static bool areEqualPoints(const Point &point1, const Point &point2) {
    return (almostEqual(point1.x, point2.x) && almostEqual(point1.y, point2.y));
}

//! Return the sign of the number
/*!
* The sign function returns:
*  -1, if number < 0
*  +1, if number > 0
*  0, otherwise
*/
static int sign(double number) {
    return (number > 0) ? 1 : ((number < 0) ? -1 : 0);
}

//! Return the maximum of the provided numbers
static double maximum(double number1, double number2, double number3) {
    return std::max(std::max(number1, number2), number3);
}

//! Check if the two numbers are equal (almost)
/*!
* The expression for determining if two real numbers are equal is:
* if (Abs(x - y) <= EPSILON * Max(1.0f, Abs(x), Abs(y))).
*
* @param number1 First number
* @param number2 Second number
*/
static bool almostEqual(double number1, double number2) {
    return (std::abs(number1 - number2) <= (EPSILON * maximum(1.0, std::abs(number1), std::abs(number2))));
}

//! Check if the first number is greater than or equal to the second number
/*!
* @param number1 First number
* @param number2 Second number
*/
static bool greaterOrEqual(double number1, double number2) {
    return ((number1 > number2) || (almostEqual(number1, number2)));
}

//! Check if the first number is less than or equal to the second number
/*!
* @param number1 First number
* @param number2 Second number
*/
static bool lessOrEqual(double number1, double number2) {
    return ((number1 < number2) || (almostEqual(number1, number2)));
}

}
