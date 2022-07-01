#ifndef _ScaleListNode
#define _ScaleListNode
//
// File: ScaleListNode.h
//
// Dependency Graph Node: scaleList
//
// Author: Benjamin H. Singleton
//

#include <utility>
#include <map>
#include <vector>

#include <maya/MPxNode.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>
#include <maya/MMatrix.h>
#include <maya/MTransformationMatrix.h>
#include <maya/MVector.h>
#include <maya/MVectorArray.h>
#include <maya/MMatrix.h>
#include <maya/MFloatArray.h>
#include <maya/MString.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnUnitAttribute.h>
#include <maya/MFnMatrixAttribute.h>
#include <maya/MFnNumericData.h>
#include <maya/MFnCompoundAttribute.h>
#include <maya/MTypeId.h> 
#include <maya/MGlobal.h>


struct ScaleListItem
{

	MString	name = "";
	float weight = 1.0;
	bool absolute = false;
	MVector scale = MVector::one;

};


class ScaleList : public MPxNode
{

public:

						ScaleList();
	virtual				~ScaleList();

	virtual MStatus		compute(const MPlug& plug, MDataBlock& data);
	static  void*		creator();
	static  MStatus		initialize();
	
	static	MVector		average(const std::vector<ScaleListItem>& items);
	static	void		normalize(std::vector<ScaleListItem>& items);

	static	MMatrix		createScaleMatrix(const double x, const double y, const double z);
	static	MMatrix		createScaleMatrix(const MVector& scale);
	
public:

	static	MObject		active;
	static	MObject		normalizeWeights;
	static	MObject		list;
	static	MObject		name;
	static	MObject		weight;
	static	MObject		absolute;
	static	MObject		scale;
	static	MObject		scaleX;
	static	MObject		scaleY;
	static	MObject		scaleZ;
	
	static	MObject		output;
	static	MObject		outputX;
	static	MObject		outputY;
	static	MObject		outputZ;
	static	MObject		matrix;
	static	MObject		inverseMatrix;

	static	MTypeId		id;
	static	MString		listCategory;
	static	MString		scaleCategory;
	static	MString		outputCategory;

};

#endif