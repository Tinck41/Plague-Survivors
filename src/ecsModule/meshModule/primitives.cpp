#include "primitives.h"

#include "components.h"

using namespace ps;

Rectangle::operator Mesh() {
	return {};
}

Circle::operator Mesh() {
	return {};
}
