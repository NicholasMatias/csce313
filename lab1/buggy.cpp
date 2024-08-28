#include <iostream>
#include <cstring>
using namespace std;
struct Point {
    int x, y;

    Point () : x(0), y(0) {}
    Point (int _x, int _y) : x(_x), y(_y) {}
};


class Shape {
// changed these to private as they should only be accessed within the class itself.    
private:
    int vertices;
    Point* points;

public:
    Shape(int _vertices) : vertices(_vertices) {
        points = new Point[vertices];
    }
    // this will delete the points in the shape so that memory is preserved following being used. 
    ~Shape() {
        delete[] points;
    }
    //adds all the points to the shape
    void addPoints(const Point pts[]) {
        for (int i = 0; i < vertices; i++) {
            points[i] = pts[i];
        }
    }

    double area() const {
        double temp = 0;
        for (int i = 0; i < vertices; i++) {
            int j = (i + 1) % vertices;
            temp += (points[i].x * points[j].y) - (points[j].x * points[i].y);
        }
        return std::abs(temp) / 2.0;
    }// make sure we are taking the absolute value. Take into account negative numbers. 
};


int main () {
    // FIXME: create the following points using the three different methods
    //        of defining structs:
    //          tri1 = (0, 0)
    //          tri2 = (1, 2)
    //          tri3 = (2, 0)
    Point tri1;
    Point tri2(1,2);
    Point* tri3 = new Point(2,0);

    // adding points to tri
    Shape* tri = new Shape(3);
    Point triPts[3] = {tri1, tri2, *tri3};
    tri->addPoints(triPts);

    // FIXME: create the following points using your preferred struct
    //        definition:
    //          quad1 = (0, 0)
    //          quad2 = (0, 2)
    //          quad3 = (2, 2)
    //          quad4 = (2, 0)

    // adding points to quad
    Point quad1(0,0);
    Point quad2(0,2);
    Point quad3(2,2);
    Point quad4(2,0);
    Shape* quad = new Shape(4);
    Point quadPts[4] = {quad1, quad2, quad3, quad4};
    quad->addPoints(quadPts);

    // FIXME: print out area of tri and area of quad
    // Print areas
    std::cout << "Area of triangle: " << tri->area() << std::endl;
    std::cout << "Area of quadrilateral: " << quad->area() << std::endl;

    //this will release the memory since we are done using them now. 
    delete tri;
    delete quad;
    delete tri3;

    return 0;
}
