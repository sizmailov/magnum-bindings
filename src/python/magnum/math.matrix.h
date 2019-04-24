#ifndef magnum_math_matrix_h
#define magnum_math_matrix_h
/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019
              Vladimír Vondruš <mosra@centrum.cz>

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
*/

#include <pybind11/pybind11.h>
#include <pybind11/operators.h>
#include <Magnum/Math/Matrix3.h>
#include <Magnum/Math/Matrix4.h>

#include "magnum/math.h"

namespace magnum {

/* A variant of Magnum's own DimensionTraits, but working for 2/3/4 dimensions
   instead of 1/2/3 dimensions */
template<UnsignedInt, class> struct VectorTraits;
template<class T> struct VectorTraits<2, T> { typedef Math::Vector2<T> Type; };
template<class T> struct VectorTraits<3, T> { typedef Math::Vector3<T> Type; };
template<class T> struct VectorTraits<4, T> { typedef Math::Vector4<T> Type; };

template<class T> void rectangularMatrix(py::class_<T>& c) {
    /*
        Missing APIs:

        from(T*)
        fromVector() (would need Vector6,...Vector16 for that)
        Type
        construction from different types
        construction by slicing or expanding differently sized matrices
        row() / setRow() (function? that's ugly. property? not sure how)
        component-wise operations (would need BoolVector6 ... BoolVector16)
        ij() (doesn't make sense in generic code as we don't have Matrix1)
    */

    c
        /* Constructors */
        .def_static("from_diagonal", [](const typename VectorTraits<T::DiagonalSize, typename T::Type>::Type& vector) {
            return T::fromDiagonal(vector);
        }, "Construct a diagonal matrix")
        .def_static("zero_init", []() {
            return T{Math::ZeroInit};
        }, "Construct a zero-filled matrix")
        .def(py::init(), "Default constructor")
        .def(py::init<typename T::Type>(), "Construct a matrix with one value for all components")

        /* Comparison */
        .def(py::self == py::self, "Equality comparison")
        .def(py::self != py::self, "Non-equality comparison")

        /* Set / get. Need to throw IndexError in order to allow iteration:
           https://docs.python.org/3/reference/datamodel.html#object.__getitem__ */
        .def("__setitem__", [](T& self, std::size_t i, const  typename VectorTraits<T::Rows, typename T::Type>::Type& value) {
            if(i >= T::Cols) throw pybind11::index_error{};
            self[i] = value;
        }, "Set a column at given position")
        .def("__getitem__", [](const T& self, std::size_t i) ->  typename VectorTraits<T::Rows, typename T::Type>::Type {
            if(i >= T::Cols) throw pybind11::index_error{};
            return self[i];
        }, "Column at given position")
        /* Set / get for direct elements, because [a][b] = 2.5 won't work
           without involving shared pointers */
        .def("__setitem__", [](T& self, const std::pair<std::size_t, std::size_t>& i, typename T::Type value) {
            if(i.first >= T::Cols || i.second >= T::Rows) throw pybind11::index_error{};
            self[i.first][i.second] = value;
        }, "Set a value at given col/row")
        .def("__getitem__", [](const T& self, const std::pair<std::size_t, std::size_t>& i) {
            if(i.first >= T::Cols || i.second >= T::Rows) throw pybind11::index_error{};
            return self[i.first][i.second];
        }, "Value at given col/row")

        /* Operators */
        .def(-py::self, "Negated matrix")
        .def(py::self += py::self, "Add and assign a matrix")
        .def(py::self + py::self, "Add a matrix")
        .def(py::self -= py::self, "Subtract and assign a matrix")
        .def(py::self - py::self, "Subtract a matrix")
        .def(py::self *= typename T::Type{}, "Multiply with a scalar and assign")
        .def(py::self * typename T::Type{}, "Multiply with a scalar")
        .def(py::self /= typename T::Type{}, "Divide with a scalar and assign")
        .def(py::self / typename T::Type{}, "Divide with a scalar")
        .def("__mul__", [](const T& self, const typename VectorTraits<T::Cols, typename T::Type>::Type& vector) -> typename VectorTraits<T::Rows, typename T::Type>::Type {
            return self*vector;
        }, "Multiply a vector")
        .def(typename T::Type{} * py::self, "Multiply a scalar with a matrix")
        .def(typename T::Type{} / py::self, "Divide a matrix with a scalar and invert")

        /* Member functions that don't return a size-dependent type */
        .def("flipped_cols", &T::flippedCols, "Matrix with flipped cols")
        .def("flipped_rows", &T::flippedRows, "Matrix with flipped rows")
        .def("diagonal", [](const T& self) -> typename VectorTraits<T::DiagonalSize, typename T::Type>::Type {
            return self.diagonal();
        }, "Values on diagonal")

        .def("__repr__", repr<T>, "Object representation");

    /* Matrix column count */
    char lenDocstring[] = "Matrix column count. Returns _.";
    lenDocstring[sizeof(lenDocstring) - 3] = '0' + T::Cols;
    c.def_static("__len__", []() { return int(T::Cols); }, lenDocstring);
}

template<class T> void matrix(py::class_<T>& c) {
    c
        /* Constructors */
        .def_static("identity_init", [](typename T::Type value) {
            return T{Math::IdentityInit, value};
        }, "Construct an identity matrix", py::arg("value") = typename T::Type(1))

        /* Member functions for square matrices only */
        .def("is_orthogonal", &T::isOrthogonal, "Whether the matrix is orthogonal")
        .def("trace", &T::trace, "Trace of the matrix")
        .def("determinant", &T::determinant, "Determinant")
        .def("inverted", &T::inverted, "Inverted matrix")
        .def("inverted_orthogonal", &T::invertedOrthogonal, "Inverted orthogonal matrix");
}

template<class T> void matrices(
    py::class_<Math::Matrix2x2<T>>& matrix2x2,
    py::class_<Math::Matrix2x3<T>>& matrix2x3,
    py::class_<Math::Matrix2x4<T>>& matrix2x4,

    py::class_<Math::Matrix3x2<T>>& matrix3x2,
    py::class_<Math::Matrix3x3<T>>& matrix3x3,
    py::class_<Math::Matrix3x4<T>>& matrix3x4,

    py::class_<Math::Matrix4x2<T>>& matrix4x2,
    py::class_<Math::Matrix4x3<T>>& matrix4x3,
    py::class_<Math::Matrix4x4<T>>& matrix4x4,

    py::class_<Math::Matrix3<T>, Math::Matrix3x3<T>>& matrix3,
    py::class_<Math::Matrix4<T>, Math::Matrix4x4<T>>& matrix4
) {
    /* Two-column matrices */
    matrix2x2
        .def(py::init<const Math::Vector2<T>&, const Math::Vector2<T>&>(),
            "Construct from column vectors")
        .def(py::init([](const std::tuple<Math::Vector2<T>, Math::Vector2<T>>& value) {
            return Math::Matrix2x2<T>{std::get<0>(value), std::get<1>(value)};
        }), "Construct from a column vector tuple")
        .def("__matmul__", [](const Math::Matrix2x2<T>& self, const Math::Matrix2x2<T>& other) -> Math::Matrix2x2<T> {
            return self*other;
        }, "Multiply a matrix")
        .def("__matmul__", [](const Math::Matrix2x2<T>& self, const Math::Matrix3x2<T>& other) -> Math::Matrix3x2<T> {
            return self*other;
        }, "Multiply a matrix")
        .def("__matmul__", [](const Math::Matrix2x2<T>& self, const Math::Matrix4x2<T>& other) -> Math::Matrix4x2<T> {
            return self*other;
        }, "Multiply a matrix")
        .def("transposed", [](const Math::Matrix2x2<T>& self) -> Math::Matrix2x2<T> {
            return self.transposed();
        }, "Transposed matrix");
    matrix2x3
        .def(py::init<const Math::Vector3<T>&, const Math::Vector3<T>&>(),
            "Construct from column vectors")
        .def(py::init([](const std::tuple<Math::Vector3<T>, Math::Vector3<T>>& value) {
            return Math::Matrix2x3<T>{std::get<0>(value), std::get<1>(value)};
        }), "Construct from a column vector tuple")
        .def("__matmul__", [](const Math::Matrix2x3<T>& self, const Math::Matrix2x2<T>& other) -> Math::Matrix2x3<T> {
            return self*other;
        }, "Multiply a matrix")
        .def("__matmul__", [](const Math::Matrix2x3<T>& self, const Math::Matrix3x2<T>& other) -> Math::Matrix3x3<T> {
            return self*other;
        }, "Multiply a matrix")
        .def("__matmul__", [](const Math::Matrix2x3<T>& self, const Math::Matrix4x2<T>& other) -> Math::Matrix4x3<T> {
            return self*other;
        }, "Multiply a matrix")
        .def("transposed", [](const Math::Matrix2x3<T>& self) -> Math::Matrix3x2<T> {
            return self.transposed();
        }, "Transposed matrix");
    matrix2x4
        .def(py::init<const Math::Vector4<T>&, const Math::Vector4<T>&>(),
            "Construct from column vectors")
        .def(py::init([](const std::tuple<Math::Vector4<T>, Math::Vector4<T>>& value) {
            return Math::Matrix2x4<T>{std::get<0>(value), std::get<1>(value)};
        }), "Construct from a column vector tuple")
        .def("__matmul__", [](const Math::Matrix2x4<T>& self, const Math::Matrix2x2<T>& other) -> Math::Matrix2x4<T> {
            return self*other;
        }, "Multiply a matrix")
        .def("__matmul__", [](const Math::Matrix2x4<T>& self, const Math::Matrix3x2<T>& other) -> Math::Matrix3x4<T> {
            return self*other;
        }, "Multiply a matrix")
        .def("__matmul__", [](const Math::Matrix2x4<T>& self, const Math::Matrix4x2<T>& other) -> Math::Matrix4x4<T> {
            return self*other;
        }, "Multiply a matrix")
        .def("transposed", [](const Math::Matrix2x4<T>& self) -> Math::Matrix4x2<T> {
            return self.transposed();
        }, "Transposed matrix");
    rectangularMatrix(matrix2x2);
    rectangularMatrix(matrix2x3);
    rectangularMatrix(matrix2x4);
    matrix(matrix2x2);

    /* Three-column matrices */
    matrix3x2
        .def(py::init<const Math::Vector2<T>&, const Math::Vector2<T>&, const Math::Vector2<T>&>(),
            "Construct from column vectors")
        .def(py::init([](const std::tuple<Math::Vector2<T>, Math::Vector2<T>, Math::Vector2<T>>& value) {
            return Math::Matrix3x2<T>{std::get<0>(value), std::get<1>(value), std::get<2>(value)};
        }), "Construct from a column vector tuple")
        .def("__matmul__", [](const Math::Matrix3x2<T>& self, const Math::Matrix2x3<T>& other) -> Math::Matrix2x2<T> {
            return self*other;
        }, "Multiply a matrix")
        .def("__matmul__", [](const Math::Matrix3x2<T>& self, const Math::Matrix3x3<T>& other) -> Math::Matrix3x2<T> {
            return self*other;
        }, "Multiply a matrix")
        .def("__matmul__", [](const Math::Matrix3x2<T>& self, const Math::Matrix4x3<T>& other) -> Math::Matrix4x2<T> {
            return self*other;
        }, "Multiply a matrix")
        .def("transposed", [](const Math::Matrix3x2<T>& self) -> Math::Matrix2x3<T> {
            return self.transposed();
        }, "Transposed matrix");
    matrix3x3
        .def(py::init<const Math::Vector3<T>&, const Math::Vector3<T>&, const Math::Vector3<T>&>(),
            "Construct from column vectors")
        .def(py::init([](const std::tuple<Math::Vector3<T>, Math::Vector3<T>, Math::Vector3<T>>& value) {
            return Math::Matrix3x3<T>{std::get<0>(value), std::get<1>(value), std::get<2>(value)};
        }), "Construct from a column vector tuple")
        .def("__matmul__", [](const Math::Matrix3x3<T>& self, const Math::Matrix2x3<T>& other) -> Math::Matrix2x3<T> {
            return self*other;
        }, "Multiply a matrix")
        .def("__matmul__", [](const Math::Matrix3x3<T>& self, const Math::Matrix3x3<T>& other) -> Math::Matrix3x3<T> {
            return self*other;
        }, "Multiply a matrix")
        .def("__matmul__", [](const Math::Matrix3x3<T>& self, const Math::Matrix4x3<T>& other) -> Math::Matrix4x3<T> {
            return self*other;
        }, "Multiply a matrix")
        .def("transposed", [](const Math::Matrix3x3<T>& self) -> Math::Matrix3x3<T> {
            return self.transposed();
        }, "Transposed matrix");
    matrix3x4
        .def(py::init<const Math::Vector4<T>&, const Math::Vector4<T>&, const Math::Vector4<T>&>(),
            "Construct from column vectors")
        .def(py::init([](const std::tuple<Math::Vector4<T>, Math::Vector4<T>, Math::Vector4<T>>& value) {
            return Math::Matrix3x4<T>{std::get<0>(value), std::get<1>(value), std::get<2>(value)};
        }), "Construct from a column vector tuple")
        .def("__matmul__", [](const Math::Matrix3x4<T>& self, const Math::Matrix2x3<T>& other) -> Math::Matrix2x4<T> {
            return self*other;
        }, "Multiply a matrix")
        .def("__matmul__", [](const Math::Matrix3x4<T>& self, const Math::Matrix3x3<T>& other) -> Math::Matrix3x4<T> {
            return self*other;
        }, "Multiply a matrix")
        .def("__matmul__", [](const Math::Matrix3x4<T>& self, const Math::Matrix4x3<T>& other) -> Math::Matrix4x4<T> {
            return self*other;
        }, "Multiply a matrix")
        .def("transposed", [](const Math::Matrix3x4<T>& self) -> Math::Matrix4x3<T> {
            return self.transposed();
        }, "Transposed matrix");
    rectangularMatrix(matrix3x2);
    rectangularMatrix(matrix3x3);
    rectangularMatrix(matrix3x4);
    matrix(matrix3x3);

    /* Four-column matrices */
    matrix4x2
        .def(py::init<const Math::Vector2<T>&, const Math::Vector2<T>&, const Math::Vector2<T>&, const Math::Vector2<T>&>(),
            "Construct from column vectors")
        .def(py::init([](const std::tuple<Math::Vector2<T>, Math::Vector2<T>, Math::Vector2<T>, Math::Vector2<T>>& value) {
            return Math::Matrix4x2<T>{std::get<0>(value), std::get<1>(value), std::get<2>(value), std::get<3>(value)};
        }), "Construct from a column vector tuple")
        .def("__matmul__", [](const Math::Matrix4x2<T>& self, const Math::Matrix2x4<T>& other) -> Math::Matrix2x2<T> {
            return self*other;
        }, "Multiply a matrix")
        .def("__matmul__", [](const Math::Matrix4x2<T>& self, const Math::Matrix3x4<T>& other) -> Math::Matrix3x2<T> {
            return self*other;
        }, "Multiply a matrix")
        .def("__matmul__", [](const Math::Matrix4x2<T>& self, const Math::Matrix4x4<T>& other) -> Math::Matrix4x2<T> {
            return self*other;
        }, "Multiply a matrix")
        .def("transposed", [](const Math::Matrix4x2<T>& self) -> Math::Matrix2x4<T> {
            return self.transposed();
        }, "Transposed matrix");
    matrix4x3
        .def(py::init<const Math::Vector3<T>&, const Math::Vector3<T>&, const Math::Vector3<T>&, const Math::Vector3<T>&>(),
            "Construct from column vectors")
        .def(py::init([](const std::tuple<Math::Vector3<T>, Math::Vector3<T>, Math::Vector3<T>, Math::Vector3<T>>& value) {
            return Math::Matrix4x3<T>{std::get<0>(value), std::get<1>(value), std::get<2>(value), std::get<3>(value)};
        }), "Construct from a column vector tuple")
        .def("__matmul__", [](const Math::Matrix4x3<T>& self, const Math::Matrix2x4<T>& other) -> Math::Matrix2x3<T> {
            return self*other;
        }, "Multiply a matrix")
        .def("__matmul__", [](const Math::Matrix4x3<T>& self, const Math::Matrix3x4<T>& other) -> Math::Matrix3x3<T> {
            return self*other;
        }, "Multiply a matrix")
        .def("__matmul__", [](const Math::Matrix4x3<T>& self, const Math::Matrix4x4<T>& other) -> Math::Matrix4x3<T> {
            return self*other;
        }, "Multiply a matrix")
        .def("transposed", [](const Math::Matrix4x3<T>& self) -> Math::Matrix3x4<T> {
            return self.transposed();
        }, "Transposed matrix");
    matrix4x4
        .def(py::init<const Math::Vector4<T>&, const Math::Vector4<T>&, const Math::Vector4<T>&, const Math::Vector4<T>&>(),
            "Construct from column vectors")
        .def(py::init([](const std::tuple<Math::Vector4<T>, Math::Vector4<T>, Math::Vector4<T>, Math::Vector4<T>>& value) {
            return Math::Matrix4x4<T>{std::get<0>(value), std::get<1>(value), std::get<2>(value), std::get<3>(value)};
        }), "Construct from a column vector tuple")
        .def("__matmul__", [](const Math::Matrix4x4<T>& self, const Math::Matrix2x4<T>& other) -> Math::Matrix2x4<T> {
            return self*other;
        }, "Multiply a matrix")
        .def("__matmul__", [](const Math::Matrix4x4<T>& self, const Math::Matrix3x4<T>& other) -> Math::Matrix3x4<T> {
            return self*other;
        }, "Multiply a matrix")
        .def("__matmul__", [](const Math::Matrix4x4<T>& self, const Math::Matrix4x4<T>& other) -> Math::Matrix4x4<T> {
            return self*other;
        }, "Multiply a matrix")
        .def("transposed", [](const Math::Matrix4x4<T>& self) -> Math::Matrix4x4<T> {
            return self.transposed();
        }, "Transposed matrix");
    rectangularMatrix(matrix4x2);
    rectangularMatrix(matrix4x3);
    rectangularMatrix(matrix4x4);
    matrix(matrix4x4);

    /* 3x3 transformation matrix */
    matrix3
        /* Constructors. The scaling() / rotation() are handled below
           as they conflict with member functions. */
        .def_static("translation", static_cast<Math::Matrix3<T>(*)(const Math::Vector2<T>&)>(&Math::Matrix3<T>::translation),
            "2D translation matrix")
        .def_static("reflection", &Math::Matrix3<T>::reflection,
            "2D reflection matrix")
        .def_static("shearing_x", &Math::Matrix3<T>::shearingX,
            "2D shearing matrix along the X axis", py::arg("amount"))
        .def_static("shearing_y", &Math::Matrix3<T>::shearingY,
            "2D shearning matrix along the Y axis", py::arg("amount"))
        .def_static("projection", &Math::Matrix3<T>::projection,
            "2D projection matrix", py::arg("size"))
        .def_static("from", static_cast<Math::Matrix3<T>(*)(const Math::Matrix2x2<T>&, const Math::Vector2<T>&)>(&Math::Matrix3<T>::from),
            "Create a matrix from a rotation/scaling part and a translation part",
            py::arg("rotation_scaling"), py::arg("translation"))
        .def_static("zero_init", []() {
            return Math::Matrix3<T>{Math::ZeroInit};
        }, "Construct a zero-filled matrix")
        .def_static("identity_init", [](T value) {
            return Math::Matrix3<T>{Math::IdentityInit, value};
        }, "Construct an identity matrix", py::arg("value") = T(1))
        .def(py::init(), "Default constructor")
        .def(py::init<T>(), "Construct a matrix with one value for all components")
        .def(py::init<const Math::Vector3<T>&, const Math::Vector3<T>&, const Math::Vector3<T>&>(),
            "Construct from column vectors")
        .def(py::init([](const std::tuple<Math::Vector3<T>, Math::Vector3<T>, Math::Vector3<T>>& value) {
            return Math::Matrix3<T>{std::get<0>(value), std::get<1>(value), std::get<2>(value)};
        }), "Construct from a column vector tuple")

        /* Member functions */
        .def("is_rigid_transformation", &Math::Matrix3<T>::isRigidTransformation,
            "Check whether the matrix represents a rigid transformation")
        .def("rotation_scaling", &Math::Matrix3<T>::rotationScaling,
            "2D rotation and scaling part of the matrix")
        .def("rotation_shear", &Math::Matrix3<T>::rotationShear,
            "2D rotation and shear part of the matrix")
        .def("rotation_normalized", &Math::Matrix3<T>::rotationNormalized,
            "2D rotation part of the matrix assuming there is no scaling")
        .def("scaling_squared", &Math::Matrix3<T>::scalingSquared,
            "Non-uniform scaling part of the matrix, squared")
        .def("uniform_scaling_squared", &Math::Matrix3<T>::uniformScalingSquared,
            "Uniform scaling part of the matrix, squared")
        .def("uniform_scaling", &Math::Matrix3<T>::uniformScaling,
            "Uniform scaling part of the matrix")
        .def("inverted_rigid", &Math::Matrix3<T>::invertedRigid,
             "Inverted rigid transformation matrix")
        .def("transform_vector", &Math::Matrix3<T>::transformVector,
            "Transform a 2D vector with the matrix")
        .def("transform_point", &Math::Matrix3<T>::transformPoint,
            "Transform a 2D point with the matrix")

        /* Properties */
        .def_property("right",
            static_cast<Math::Vector2<T>(Math::Matrix3<T>::*)() const>(&Math::Matrix3<T>::right),
            [](Math::Matrix3<T>& self, const Math::Vector2<T>& value) { self.right() = value; },
            "Right-pointing 2D vector")
        .def_property("up",
            static_cast<Math::Vector2<T>(Math::Matrix3<T>::*)() const>(&Math::Matrix3<T>::up),
            [](Math::Matrix3<T>& self, const Math::Vector2<T>& value) { self.up() = value; },
            "Up-pointing 2D vector")

        /* Static/member scaling(). Pybind doesn't support that natively, so
           we create a scaling(*args, **kwargs) and dispatch ourselves. */
        .def_static("_sscaling", static_cast<Math::Matrix3<T>(*)(const Math::Vector2<T>&)>(&Math::Matrix3<T>::scaling),
            "2D scaling matrix")
        .def("_iscaling", static_cast<Math::Vector2<T>(Math::Matrix3<T>::*)() const>(&Math::Matrix3<T>::scaling),
            "Non-uniform scaling part of the matrix")
        .def("scaling", [matrix3](py::args args, py::kwargs kwargs) {
            if(py::len(args) && py::isinstance<Math::Matrix3<T>>(args[0])) {
                return matrix3.attr("_iscaling")(*args, **kwargs);
            } else {
                return matrix3.attr("_sscaling")(*args, **kwargs);
            }
        })

        /* Static/member rotation(). Pybind doesn't support that natively, so
           we create a rotation(*args, **kwargs) and dispatch ourselves. */
        .def_static("_srotation", [](Radd angle) {
            return Math::Matrix3<T>::rotation(Math::Rad<T>(angle));
        }, "2D rotation matrix")
        .def("_irotation", static_cast<Math::Matrix2x2<T>(Math::Matrix3<T>::*)() const>(&Math::Matrix3<T>::rotation),
            "2D rotation part of the matrix")
        .def("rotation", [matrix3](py::args args, py::kwargs kwargs) {
            if(py::len(args) && py::isinstance<Math::Matrix3<T>>(args[0])) {
                return matrix3.attr("_irotation")(*args, **kwargs);
            } else {
                return matrix3.attr("_srotation")(*args, **kwargs);
            }
        });

    /* 4x4 transformation matrix */
    matrix4
        /* Constructors. The scaling() / rotation() are handled below
           as they conflict with member functions. */
        .def_static("translation", static_cast<Math::Matrix4<T>(*)(const Math::Vector3<T>&)>(&Math::Matrix4<T>::translation),
            "3D translation matrix")
        .def_static("rotation_x", [](Radd angle) {
            return Math::Matrix4<T>::rotationX(Math::Rad<T>(angle));
        }, "3D rotation matrix around the X axis")
        .def_static("rotation_y", [](Radd angle) {
            return Math::Matrix4<T>::rotationY(Math::Rad<T>(angle));
        }, "3D rotation matrix around the Y axis")
        .def_static("rotation_z", [](Radd angle) {
            return Math::Matrix4<T>::rotationZ(Math::Rad<T>(angle));
        }, "3D rotation matrix around the Z axis")
        .def_static("reflection", &Math::Matrix4<T>::reflection,
            "3D reflection matrix")
        .def_static("shearing_xy", &Math::Matrix4<T>::shearingXY,
            "3D shearing matrix along the XY plane", py::arg("amountx"), py::arg("amounty"))
        .def_static("shearing_xz", &Math::Matrix4<T>::shearingXZ,
            "3D shearning matrix along the XZ plane", py::arg("amountx"), py::arg("amountz"))
        .def_static("shearing_yz", &Math::Matrix4<T>::shearingYZ,
            "3D shearing matrix along the YZ plane", py::arg("amounty"), py::arg("amountz"))
        .def_static("orthographic_projection", &Math::Matrix4<T>::orthographicProjection,
            "3D orthographic projection matrix", py::arg("size"), py::arg("near"), py::arg("far"))
        .def_static("perspective_projection",
            static_cast<Math::Matrix4<T>(*)(const Math::Vector2<T>&, T, T)>(&Math::Matrix4<T>::perspectiveProjection),
            "3D perspective projection matrix", py::arg("size"), py::arg("near"), py::arg("far"))
        .def_static("perspective_projection", [](Radd fov, T aspectRatio, T near, T far) {
            return Math::Matrix4<T>::perspectiveProjection(Math::Rad<T>(fov), aspectRatio, near, far);
        }, "3D perspective projection matrix", py::arg("fov"), py::arg("aspect_ratio"), py::arg("near"), py::arg("far"))
        .def_static("look_at", &Math::Matrix4<T>::lookAt,
            "Matrix oriented towards a specific point", py::arg("eye"), py::arg("target"), py::arg("up"))
        .def_static("from", static_cast<Math::Matrix4<T>(*)(const Math::Matrix3x3<T>&, const Math::Vector3<T>&)>(&Math::Matrix4<T>::from),
            "Create a matrix from a rotation/scaling part and a translation part",
            py::arg("rotation_scaling"), py::arg("translation"))
        .def_static("zero_init", []() {
            return Math::Matrix4<T>{Math::ZeroInit};
        }, "Construct a zero-filled matrix")
        .def_static("identity_init", [](T value) {
            return Math::Matrix4<T>{Math::IdentityInit, value};
        }, "Construct an identity matrix", py::arg("value") = T(1))
        .def(py::init(), "Default constructor")
        .def(py::init<T>(), "Construct a matrix with one value for all components")
        .def(py::init<const Math::Vector4<T>&, const Math::Vector4<T>&, const Math::Vector4<T>&, const Math::Vector4<T>&>(),
            "Construct from column vectors")
        .def(py::init([](const std::tuple<Math::Vector4<T>, Math::Vector4<T>, Math::Vector4<T>, Math::Vector4<T>>& value) {
            return Math::Matrix4<T>{std::get<0>(value), std::get<1>(value), std::get<2>(value), std::get<3>(value)};
        }), "Construct from a column vector tuple")

        /* Member functions */
        .def("is_rigid_transformation", &Math::Matrix4<T>::isRigidTransformation,
            "Check whether the matrix represents a rigid transformation")
        .def("rotation_scaling", &Math::Matrix4<T>::rotationScaling,
            "3D rotation and scaling part of the matrix")
        .def("rotation_shear", &Math::Matrix4<T>::rotationShear,
            "3D rotation and shear part of the matrix")
        .def("rotation_normalized", &Math::Matrix4<T>::rotationNormalized,
            "3D rotation part of the matrix assuming there is no scaling")
        .def("scaling_squared", &Math::Matrix4<T>::scalingSquared,
            "Non-uniform scaling part of the matrix, squared")
        .def("uniform_scaling_squared", &Math::Matrix4<T>::uniformScalingSquared,
            "Uniform scaling part of the matrix, squared")
        .def("uniform_scaling", &Math::Matrix4<T>::uniformScaling,
            "Uniform scaling part of the matrix")
        .def("inverted_rigid", &Math::Matrix4<T>::invertedRigid,
             "Inverted rigid transformation matrix")
        .def("transform_vector", &Math::Matrix4<T>::transformVector,
            "Transform a 3D vector with the matrix")
        .def("transform_point", &Math::Matrix4<T>::transformPoint,
            "Transform a 3D point with the matrix")

        /* Properties */
        .def_property("right",
            static_cast<Math::Vector3<T>(Math::Matrix4<T>::*)() const>(&Math::Matrix4<T>::right),
            [](Math::Matrix4<T>& self, const Math::Vector3<T>& value) { self.right() = value; },
            "Right-pointing 3D vector")
        .def_property("up",
            static_cast<Math::Vector3<T>(Math::Matrix4<T>::*)() const>(&Math::Matrix4<T>::up),
            [](Math::Matrix4<T>& self, const Math::Vector3<T>& value) { self.up() = value; },
            "Up-pointing 3D vector")
        .def_property("backward",
            static_cast<Math::Vector3<T>(Math::Matrix4<T>::*)() const>(&Math::Matrix4<T>::backward),
            [](Math::Matrix4<T>& self, const Math::Vector3<T>& value) { self.backward() = value; },
            "Backward-pointing 3D vector")

        /* Static/member scaling(). Pybind doesn't support that natively, so
           we create a scaling(*args, **kwargs) and dispatch ourselves. */
        .def_static("_sscaling", static_cast<Math::Matrix4<T>(*)(const Math::Vector3<T>&)>(&Math::Matrix4<T>::scaling),
            "3D scaling matrix")
        .def("_iscaling", static_cast<Math::Vector3<T>(Math::Matrix4<T>::*)() const>(&Math::Matrix4<T>::scaling),
            "Non-uniform scaling part of the matrix")
        .def("scaling", [matrix4](py::args args, py::kwargs kwargs) {
            if(py::len(args) && py::isinstance<Math::Matrix4<T>>(args[0])) {
                return matrix4.attr("_iscaling")(*args, **kwargs);
            } else {
                return matrix4.attr("_sscaling")(*args, **kwargs);
            }
        })

        /* Static/member rotation(). Pybind doesn't support that natively, so
           we create a rotation(*args, **kwargs) and dispatch ourselves. */
        .def_static("_srotation", [](Radd angle, const Math::Vector3<T>& axis) {
            return Math::Matrix4<T>::rotation(Math::Rad<T>(angle), axis);
        }, "3D rotation matrix around arbitrary axis")
        .def("_irotation", static_cast<Math::Matrix3x3<T>(Math::Matrix4<T>::*)() const>(&Math::Matrix4<T>::rotation),
            "3D rotation part of the matrix")
        .def("rotation", [matrix4](py::args args, py::kwargs kwargs) {
            if(py::len(args) && py::isinstance<Math::Matrix4<T>>(args[0])) {
                return matrix4.attr("_irotation")(*args, **kwargs);
            } else {
                return matrix4.attr("_srotation")(*args, **kwargs);
            }
        });
}

}

#endif