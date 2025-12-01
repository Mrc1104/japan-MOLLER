#include "QwRootWriter.h"
#include<iostream>

VQwRootWriter::VQwRootWriter(std::shared_ptr<TDirectory> sink, std::string name, std::string desc, std::string prefix)
: fFile(sink)
, fName(std::move(name))
, fDesc(std::move(desc))
, fPrefix(std::move(prefix))
, fType("type undefined")
{ }

bool VQwRootWriter::IsInit() { return  fWriterInitialized & fWriterFinalized; }
bool VQwRootWriter::Initialize()
{
	if( fWriterFinalized ) throw std::runtime_error("Writer is already initialized");
	fWriterInitialized = false;
	return InitializeImpl();
}

bool VQwRootWriter::Finalize()
{
	if( IsInit() ) throw std::runtime_error("Writer is already initialized");
	fWriterFinalized = false;
	return FinalizeImpl();
}

void VQwRootWriter::AddField(QwRootTreeBranchVector &values)
{
	if( !fWriterInitialized )
		throw std::runtime_error("Adding a field to an Unitialized Writer!");
	if(  fWriterFinalized   ) {
		std::cout << "Writer already Finalized, ignoring\n";
		return;
	}
	fBranches.emplace_back( &values );
	AddFieldImpl(values);
}

void VQwRootWriter::AddFields(std::vector<QwRootTreeBranchVector*> values)
{
	if( !fWriterInitialized )
		throw std::runtime_error("Adding a field to an Unitialized Writer!");
	if(  fWriterFinalized   ) {
		std::cout << "Writer already Finalized, ignoring\n";
		return;
	}
	fBranches.insert( std::end(fBranches), std::begin(values), std::end(values));
	for( auto branch : values )
		AddFieldImpl( *branch );
}


std::size_t VQwRootWriter::Fill()
{
	if(!fWriterInitialized)
		throw std::runtime_error("Trying to fill an an Unitialized Writer!");
	if(!fWriterFinalized)
		throw std::runtime_error("Trying to fill an an UnFinalized Writer!");
	return FillImpl();
}
