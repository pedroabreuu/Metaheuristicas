#include <algorithm>
#include <vector>
#include <utility>
#include "hilbert.h"

std::vector<int> hilberTransform(const std::vector<Node>& nodes, int p) {
	int xMin = nodes[0].x;
	int xMax = nodes[0].x;
	int yMin = nodes[0].y;
	int yMax = nodes[0].y;

	int amplitudeX = 0;
	int amplitudeY = 0;

	int fatorEscala = 0;

	std::vector<int> indexHilbert;

	for (const auto& node: nodes) {
		if (node.x < xMin) {
		xMin = node.x;
		}
		if (node.x > xMax) {
			xMax = node.x;
		}
		if (node.y < yMin) {
			yMin = node.y;
		}
		if (node.y > yMax) {
			yMax = node.y;
		}	
	}

	amplitudeX = xMax - xMin;
	amplitudeY = yMax - yMin;

	fatorEscala = std::max(amplitudeX, amplitudeY);

	for (const auto& node: nodes) {
		double normX = static_cast<double>(node.x - xMin) / fatorEscala;
		double normY = static_cast<double>(node.y - yMin) / fatorEscala;

		int gx = static_cast<int>(normX * ((1 << p) - 1));
		int gy = static_cast<int>(normY * ((1 << p) - 1));

		int index = 0;
		int step = (1 << p) / 2;

		while (step > 0) {
			int rx = (gx & step) > 0 ? 1 : 0;
			int ry = (gy & step) > 0 ? 1 : 0;

			index += step*step*((3*rx) ^ ry);

			if (ry == 0) {
				if (rx == 1) {
					gx = step - 1 - gx;
					gy = step - 1 - gy; 
				}
				std::swap(gx, gy);
			}
			step /= 2;
		}
		indexHilbert.push_back(index);
	}
	return indexHilbert;
}