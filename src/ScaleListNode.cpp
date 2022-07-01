//
// File: ScaleListNode.cpp
//
// Dependency Graph Node: scaleList
//
// Author: Benjamin H. Singleton
//

#include "ScaleListNode.h"

MObject		ScaleList::active;
MObject		ScaleList::normalizeWeights;
MObject		ScaleList::list;
MObject		ScaleList::name;
MObject		ScaleList::weight;
MObject		ScaleList::absolute;
MObject		ScaleList::scale;
MObject		ScaleList::scaleX;
MObject		ScaleList::scaleY;
MObject		ScaleList::scaleZ;

MObject		ScaleList::output;
MObject		ScaleList::outputX;
MObject		ScaleList::outputY;
MObject		ScaleList::outputZ;
MObject		ScaleList::matrix;
MObject		ScaleList::inverseMatrix;

MTypeId		ScaleList::id(0x0013b1c7);
MString		ScaleList::listCategory("List");
MString		ScaleList::scaleCategory("Scale");
MString		ScaleList::outputCategory("Output");


ScaleList::ScaleList() {}
ScaleList::~ScaleList() {}


MStatus ScaleList::compute(const MPlug& plug, MDataBlock& data) 
/**
This method should be overridden in user defined nodes.
Recompute the given output based on the nodes inputs.
The plug represents the data value that needs to be recomputed, and the data block holds the storage for all of the node's attributes.
The MDataBlock will provide smart handles for reading and writing this node's attribute values.
Only these values should be used when performing computations!

@param plug: Plug representing the attribute that needs to be recomputed.
@param data: Data block containing storage for the node's attributes.
@return: Return status.
*/
{

	MStatus status;

	// Check requested attribute
	//
	MObject attribute = plug.attribute(&status);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	MFnAttribute fnAttribute(attribute, &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	if (fnAttribute.hasCategory(ScaleList::outputCategory))
	{
		
		// Get input data handles
		//
		MDataHandle activeHandle = data.inputValue(ScaleList::active, &status);
		CHECK_MSTATUS_AND_RETURN_IT(status);

		MDataHandle normalizeWeightsHandle = data.inputValue(ScaleList::normalizeWeights, &status);
		CHECK_MSTATUS_AND_RETURN_IT(status);

		MArrayDataHandle listHandle = data.inputArrayValue(ScaleList::list, &status);
		CHECK_MSTATUS_AND_RETURN_IT(status);

		// Get values from handles
		//
		short active = activeHandle.asShort();
		bool normalizeWeights = normalizeWeightsHandle.asBool();
		
		// Collect scale entries
		//
		unsigned int listCount = listHandle.elementCount();
		std::vector<ScaleListItem> items = std::vector<ScaleListItem>(listCount);

		MDataHandle elementHandle, nameHandle, weightHandle, absoluteHandle, scaleHandle, scaleXHandle, scaleYHandle, scaleZHandle;
		MString name;
		float weight;
		bool absolute;
		double scaleX, scaleY, scaleZ;
		
		for (unsigned int i = 0; i < listCount; i++)
		{
			
			// Jump to array element
			//
			status = listHandle.jumpToElement(i);
			CHECK_MSTATUS_AND_RETURN_IT(status)

			elementHandle = listHandle.inputValue(&status);
			CHECK_MSTATUS_AND_RETURN_IT(status);

			// Get element data handles
			//
			nameHandle = elementHandle.child(ScaleList::name);
			weightHandle = elementHandle.child(ScaleList::weight);
			absoluteHandle = elementHandle.child(ScaleList::absolute);
			scaleHandle = elementHandle.child(ScaleList::scale);
			scaleXHandle = scaleHandle.child(ScaleList::scaleX);
			scaleYHandle = scaleHandle.child(ScaleList::scaleY);
			scaleZHandle = scaleHandle.child(ScaleList::scaleZ);
			
			// Get values from handles
			//
			name = nameHandle.asString();
			weight = weightHandle.asFloat();
			absolute = absoluteHandle.asBool();
			scaleX = scaleXHandle.asDouble();
			scaleY = scaleYHandle.asDouble();
			scaleZ = scaleZHandle.asDouble();
			
			// Assign value to arrays
			//
			items[i] = ScaleListItem{ name, weight, absolute, MVector(scaleX, scaleY, scaleZ) };
			
		}
		
		// Check if weights should be normalized
		//
		if (normalizeWeights)
		{

			ScaleList::normalize(items);

		}

		// Calculate weighted average
		//
		MVector scale = ScaleList::average(items);
		MMatrix matrix = ScaleList::createScaleMatrix(scale);
		
		// Get output data handles
		//
		MDataHandle outputXHandle = data.outputValue(ScaleList::outputX, &status);
		CHECK_MSTATUS_AND_RETURN_IT(status);

		MDataHandle outputYHandle = data.outputValue(ScaleList::outputY, &status);
		CHECK_MSTATUS_AND_RETURN_IT(status);

		MDataHandle outputZHandle = data.outputValue(ScaleList::outputZ, &status);
		CHECK_MSTATUS_AND_RETURN_IT(status);

		MDataHandle matrixHandle = data.outputValue(ScaleList::matrix, &status);
		CHECK_MSTATUS_AND_RETURN_IT(status);

		MDataHandle inverseMatrixHandle = data.outputValue(ScaleList::inverseMatrix, &status);
		CHECK_MSTATUS_AND_RETURN_IT(status);

		// Update output data handles
		//
		outputXHandle.setDouble(scale.x);
		outputXHandle.setClean();

		outputYHandle.setDouble(scale.y);
		outputYHandle.setClean();

		outputZHandle.setDouble(scale.z);
		outputZHandle.setClean();

		matrixHandle.setMMatrix(matrix);
		matrixHandle.setClean();

		inverseMatrixHandle.setMMatrix(matrix.inverse());
		inverseMatrixHandle.setClean();

		// Mark plug as clean
		//
		status = data.setClean(plug);
		CHECK_MSTATUS_AND_RETURN_IT(status);

		return MS::kSuccess;

	}
	else
	{

		return MS::kUnknownParameter;

	}

};


template<class N> N lerp(const N& start, const N& end, const double weight)
/**
Linearly interpolates the two given numbers using the supplied weight.

@param start: The start number.
@param end: The end number.
@param weight: The amount to blend.
@return: The interpolated value.
*/
{

	return (start * (1.0 - weight)) + (end * weight);

};


MVector ScaleList::average(const std::vector<ScaleListItem>& items)
/**
Returns the weighted average of the supplied scale items.

@param items: The scale items to average.
@return: Weighted average vector.
*/
{
	
	// Evaluate item count
	//
	unsigned long itemCount = items.size();
	MVector average = MVector(MVector::one);

	if (itemCount == 0)
	{

		return average;

	}
	
	// Calculate weighted average
	//
	for (ScaleListItem item : items)
	{
		
		// Evaluate which method to use
		//
		if (item.absolute)
		{

			average = lerp(average, item.scale, item.weight);

		}
		else if (abs(item.weight) > DBL_MIN)
		{

			average.x *= (item.scale.x * item.weight);
			average.y *= (item.scale.y * item.weight);
			average.z *= (item.scale.z * item.weight);

		}
		else
		{

			continue;

		}

	}
	
	return average;
	
}


void ScaleList::normalize(std::vector<ScaleListItem>& items)
/**
Normalizes the passed weights so that the total sum equals 1.0.

@param items: The items to normalize.
@return: void
*/
{

	// Get weight sum
	//
	unsigned long itemCount = items.size();
	float sum = 0.0;

	for (unsigned long i = 0; i < itemCount; i++)
	{

		sum += std::fabs(items[i].weight);

	}

	// Check for divide by zero errors!
	//
	if (sum == 0.0 || sum == 1.0)
	{

		return;

	}

	// Multiply weights by scale factor
	//
	float factor = 1.0 / sum;

	for (unsigned long i = 0; i < itemCount; i++)
	{

		items[i].weight *= factor;

	}

};


MMatrix ScaleList::createScaleMatrix(const double x, const double y, const double z)
/**
Returns a scale matrix from the supplied XYZ values.

@param x: The X value.
@param x: The Y value.
@param x: The Z value.
@return: The new scale matrix.
*/
{

	double matrix[4][4] = {
		{ x, 0.0, 0.0, 0.0 },
		{ 0.0, y, 0.0, 0.0 },
		{ 0.0, 0.0, z, 0.0 },
		{ 0.0, 0.0, 0.0, 1.0 },
	};

	return MMatrix(matrix);

};


MMatrix ScaleList::createScaleMatrix(const MVector& scale)
/**
Returns a scale matrix from the supplied vector.

@param scale: The vector to convert.
@return: The new scale matrix.
*/
{

	return ScaleList::createScaleMatrix(scale.x, scale.y, scale.z);

};


void* ScaleList::creator() 
/**
This function is called by Maya when a new instance is requested.
See pluginMain.cpp for details.

@return: ScaleList
*/
{

	return new ScaleList();

};


MStatus ScaleList::initialize()
/**
This function is called by Maya after a plugin has been loaded.
Use this function to define any static attributes.

@return: MStatus
*/
{
	
	MStatus status;

	// Initialize function sets
	//
	MFnNumericAttribute fnNumericAttr;
	MFnTypedAttribute fnTypedAttr;
	MFnUnitAttribute fnUnitAttr;
	MFnMatrixAttribute fnMatrixAttr;
	MFnCompoundAttribute fnCompoundAttr;

	// Input attributes:
	// ".active" attribute
	//
	ScaleList::active = fnNumericAttr.create("active", "a", MFnNumericData::kInt, 0, &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	// ".normalizeWeights" attribute
	//
	ScaleList::normalizeWeights = fnNumericAttr.create("normalizeWeights", "nw", MFnNumericData::kBoolean, false, &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	// ".name" attribute
	//
	ScaleList::name = fnTypedAttr.create("name", "n", MFnData::kString, &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	CHECK_MSTATUS(fnTypedAttr.addToCategory(ScaleList::listCategory));

	// ".weight" attribute
	//
	ScaleList::weight = fnNumericAttr.create("weight", "w", MFnNumericData::kFloat, 1.0, &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	
	CHECK_MSTATUS(fnNumericAttr.setMin(-1.0));
	CHECK_MSTATUS(fnNumericAttr.setMax(1.0));
	CHECK_MSTATUS(fnNumericAttr.addToCategory(ScaleList::listCategory));

	// ".absolute" attribute
	//
	ScaleList::absolute = fnNumericAttr.create("absolute", "abs", MFnNumericData::kBoolean, false, &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	CHECK_MSTATUS(fnNumericAttr.addToCategory(ScaleList::listCategory));


	// ".scaleX" attribute
	//
	ScaleList::scaleX = fnNumericAttr.create("scaleX", "sx", MFnNumericData::kDouble, 1.0, &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	CHECK_MSTATUS(fnNumericAttr.addToCategory(ScaleList::scaleCategory));
	CHECK_MSTATUS(fnNumericAttr.addToCategory(ScaleList::listCategory));

	// ".scaleY" attribute
	//
	ScaleList::scaleY = fnNumericAttr.create("scaleY", "sy",  MFnNumericData::kDouble, 1.0, &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	CHECK_MSTATUS(fnNumericAttr.addToCategory(ScaleList::scaleCategory));
	CHECK_MSTATUS(fnNumericAttr.addToCategory(ScaleList::listCategory));

	// ".scaleZ" attribute
	//
	ScaleList::scaleZ = fnNumericAttr.create("scaleZ", "sz",  MFnNumericData::kDouble, 1.0, &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	CHECK_MSTATUS(fnNumericAttr.addToCategory(ScaleList::scaleCategory));
	CHECK_MSTATUS(fnNumericAttr.addToCategory(ScaleList::listCategory));

	// ".position" attribute
	//
	ScaleList::scale = fnNumericAttr.create("scale", "s", ScaleList::scaleX, ScaleList::scaleY, ScaleList::scaleZ, &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	CHECK_MSTATUS(fnNumericAttr.addToCategory(ScaleList::scaleCategory));
	CHECK_MSTATUS(fnNumericAttr.addToCategory(ScaleList::listCategory));

	// ".list" attribute
	//
	ScaleList::list = fnCompoundAttr.create("list", "l", &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	
	CHECK_MSTATUS(fnCompoundAttr.addChild(ScaleList::name));
	CHECK_MSTATUS(fnCompoundAttr.addChild(ScaleList::weight));
	CHECK_MSTATUS(fnCompoundAttr.addChild(ScaleList::absolute));
	CHECK_MSTATUS(fnCompoundAttr.addChild(ScaleList::scale));
	CHECK_MSTATUS(fnCompoundAttr.setArray(true));
	CHECK_MSTATUS(fnCompoundAttr.addToCategory(ScaleList::listCategory));

	// Output attributes:
	// ".outputX" attribute
	//
	ScaleList::outputX = fnNumericAttr.create("outputX", "ox", MFnNumericData::kDouble, 1.0, &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	CHECK_MSTATUS(fnNumericAttr.setWritable(false));
	CHECK_MSTATUS(fnNumericAttr.setStorable(false));
	CHECK_MSTATUS(fnNumericAttr.addToCategory(ScaleList::outputCategory));

	// ".outputY" attribute
	//
	ScaleList::outputY = fnNumericAttr.create("outputY", "oy", MFnNumericData::kDouble, 1.0, &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	CHECK_MSTATUS(fnNumericAttr.setWritable(false));
	CHECK_MSTATUS(fnNumericAttr.setStorable(false));
	CHECK_MSTATUS(fnNumericAttr.addToCategory(ScaleList::outputCategory));

	// ".outputZ" attribute
	//
	ScaleList::outputZ = fnNumericAttr.create("outputZ", "oz", MFnNumericData::kDouble, 1.0, &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	CHECK_MSTATUS(fnNumericAttr.setWritable(false));
	CHECK_MSTATUS(fnNumericAttr.setStorable(false));
	CHECK_MSTATUS(fnNumericAttr.addToCategory(ScaleList::outputCategory));

	// ".output" attribute
	//
	ScaleList::output = fnNumericAttr.create("output", "o", ScaleList::outputX, ScaleList::outputY, ScaleList::outputZ, &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	CHECK_MSTATUS(fnNumericAttr.setWritable(false));
	CHECK_MSTATUS(fnNumericAttr.setStorable(false));
	CHECK_MSTATUS(fnNumericAttr.addToCategory(ScaleList::outputCategory));

	// ".matrix" attribute
	//
	ScaleList::matrix = fnMatrixAttr.create("matrix", "m", MFnMatrixAttribute::kDouble, &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	CHECK_MSTATUS(fnMatrixAttr.setWritable(false));
	CHECK_MSTATUS(fnMatrixAttr.setStorable(false));
	CHECK_MSTATUS(fnMatrixAttr.addToCategory(ScaleList::outputCategory));

	// ".inverseMatrix" attribute
	//
	ScaleList::inverseMatrix = fnMatrixAttr.create("inverseMatrix", "im", MFnMatrixAttribute::kDouble, &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	CHECK_MSTATUS(fnMatrixAttr.setWritable(false));
	CHECK_MSTATUS(fnMatrixAttr.setStorable(false));
	CHECK_MSTATUS(fnMatrixAttr.addToCategory(ScaleList::outputCategory));


	// Add attributes to node
	//
	CHECK_MSTATUS(ScaleList::addAttribute(ScaleList::active));
	CHECK_MSTATUS(ScaleList::addAttribute(ScaleList::normalizeWeights));
	CHECK_MSTATUS(ScaleList::addAttribute(ScaleList::list));

	CHECK_MSTATUS(ScaleList::addAttribute(ScaleList::output));
	CHECK_MSTATUS(ScaleList::addAttribute(ScaleList::matrix));
	CHECK_MSTATUS(ScaleList::addAttribute(ScaleList::inverseMatrix));

	// Define attribute relationships
	//
	CHECK_MSTATUS(ScaleList::attributeAffects(ScaleList::active, ScaleList::outputX));
	CHECK_MSTATUS(ScaleList::attributeAffects(ScaleList::normalizeWeights, ScaleList::outputX));
	CHECK_MSTATUS(ScaleList::attributeAffects(ScaleList::weight, ScaleList::outputX));
	CHECK_MSTATUS(ScaleList::attributeAffects(ScaleList::absolute, ScaleList::outputX));
	CHECK_MSTATUS(ScaleList::attributeAffects(ScaleList::scaleX, ScaleList::outputX));

	CHECK_MSTATUS(ScaleList::attributeAffects(ScaleList::active, ScaleList::outputY));
	CHECK_MSTATUS(ScaleList::attributeAffects(ScaleList::normalizeWeights, ScaleList::outputY));
	CHECK_MSTATUS(ScaleList::attributeAffects(ScaleList::weight, ScaleList::outputY));
	CHECK_MSTATUS(ScaleList::attributeAffects(ScaleList::absolute, ScaleList::outputY));
	CHECK_MSTATUS(ScaleList::attributeAffects(ScaleList::scaleY, ScaleList::outputY));

	CHECK_MSTATUS(ScaleList::attributeAffects(ScaleList::active, ScaleList::outputZ));
	CHECK_MSTATUS(ScaleList::attributeAffects(ScaleList::normalizeWeights, ScaleList::outputZ));
	CHECK_MSTATUS(ScaleList::attributeAffects(ScaleList::weight, ScaleList::outputZ));
	CHECK_MSTATUS(ScaleList::attributeAffects(ScaleList::absolute, ScaleList::outputZ));
	CHECK_MSTATUS(ScaleList::attributeAffects(ScaleList::scaleX, ScaleList::outputZ));
	CHECK_MSTATUS(ScaleList::attributeAffects(ScaleList::scaleY, ScaleList::outputZ));
	CHECK_MSTATUS(ScaleList::attributeAffects(ScaleList::scaleZ, ScaleList::outputZ));

	CHECK_MSTATUS(ScaleList::attributeAffects(ScaleList::active, ScaleList::matrix));
	CHECK_MSTATUS(ScaleList::attributeAffects(ScaleList::normalizeWeights, ScaleList::matrix));
	CHECK_MSTATUS(ScaleList::attributeAffects(ScaleList::weight, ScaleList::matrix));
	CHECK_MSTATUS(ScaleList::attributeAffects(ScaleList::absolute, ScaleList::matrix));
	CHECK_MSTATUS(ScaleList::attributeAffects(ScaleList::scaleZ, ScaleList::matrix));

	CHECK_MSTATUS(ScaleList::attributeAffects(ScaleList::active, ScaleList::inverseMatrix));
	CHECK_MSTATUS(ScaleList::attributeAffects(ScaleList::normalizeWeights, ScaleList::inverseMatrix));
	CHECK_MSTATUS(ScaleList::attributeAffects(ScaleList::weight, ScaleList::inverseMatrix));
	CHECK_MSTATUS(ScaleList::attributeAffects(ScaleList::absolute, ScaleList::inverseMatrix));
	CHECK_MSTATUS(ScaleList::attributeAffects(ScaleList::scaleX, ScaleList::inverseMatrix));
	CHECK_MSTATUS(ScaleList::attributeAffects(ScaleList::scaleY, ScaleList::inverseMatrix));
	CHECK_MSTATUS(ScaleList::attributeAffects(ScaleList::scaleZ, ScaleList::inverseMatrix));

	return status;

};