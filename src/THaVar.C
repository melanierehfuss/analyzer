//*-- Author :    Ole Hansen   26/04/2000


//////////////////////////////////////////////////////////////////////////
//
// THaVar
//
// A "global variable".  This is essentially a read-only pointer to the 
// actual data, along with support for data types and arrays.  It can
// be used to retrieve data from anywhere in memory, e.g. analysis results 
// of some detector object.
//
// THaVar objects are managed by the THaVarList.
// 
// The standard way to define a variable is via the constructor, e.g.
//
//    Double_t x = 1.5;
//    THaVar v("x","variable x",x);
//
// Constructors and Set() determine the data type automatically.
// Supported types are 
//    Double_t, Float_t, Long_t, ULong_t, Int_t, UInt_t, 
//    Short_t, UShort_t, Char_t, Byte_t
//
// Arrays can be defined as follows:
//
//    Double_t* a = new Double_t[10];
//    THaVar* va = new THaVar("a[10]","array of doubles",a);
//
// One-dimensional variable-size arrays are also supported. The actual
// size must be contained in a variable of type Int_t. 
// NOTE: This feature may still be buggy. Use with caution.
// Example:
//
//    Double_t* a = new Double_t[20];
//    Int_t size;
//    GetValues( a, size );  //Fills a and sets size to number of data.
//    THaVar* va = new THaVar("a","var size array",a,&size);
//
// Any array size given in the name string is ignored (e.g. "a[20]"
// would have the same effect as "a"). The data are always interpreted
// as a one-dimensional array.
//
// Data should normally be retrieved using GetValue(), which always
// returns Double_t.  
// If no type conversion is desired, or for maximum
// efficiency, one can use the typed GetValue methods.  However, this
// should be done with extreme care; a type mismatch could return
// just garbage without warning.
//
//////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <cstring>
#include "THaVar.h"
#include "TMethodCall.h"
#include "TSeqCollection.h"

ClassImp(THaVar)

//_____________________________________________________________________________
THaVar::THaVar( const THaVar& rhs ) :
  TNamed( rhs ), fArrayData(rhs.fArrayData), fValueD(rhs.fValueD),
  fType(rhs.fType), fCount(rhs.fCount), fOffset(rhs.fOffset)
{
  // Copy constructor

  if( rhs.fMethod )
    fMethod = new TMethodCall( *rhs.fMethod );
  else
    fMethod = NULL;
}

//_____________________________________________________________________________
THaVar& THaVar::operator=( const THaVar& rhs )
{
  // Assignment operator. Must be explicit due to limitations of CINT.

  if( this != &rhs ) {
    TNamed::operator=(rhs);
    fValueD    = rhs.fValueD;  // assignment of pointer ok in this case
    fArrayData = rhs.fArrayData;
    fType      = rhs.fType;
    fCount     = rhs.fCount;
    fOffset    = rhs.fOffset;
    if( rhs.fMethod )
      fMethod  = new TMethodCall( *rhs.fMethod );
    else
      fMethod  = NULL;
  }
  return *this;
}

//_____________________________________________________________________________
void THaVar::Copy( TObject& rhs )
{
  // Copy this object to rhs

  TNamed::Copy(rhs);
  ((THaVar&)rhs).fValueD    = fValueD;
  ((THaVar&)rhs).fArrayData = fArrayData;
  ((THaVar&)rhs).fType      = fType;
  ((THaVar&)rhs).fCount     = fCount;
  ((THaVar&)rhs).fOffset    = fOffset;
  if( fMethod )
    ((THaVar&)rhs).fMethod  = new TMethodCall( *fMethod );
  else
    ((THaVar&)rhs).fMethod  = NULL;
}

//_____________________________________________________________________________
THaVar::~THaVar()
{
  // Destructor

  delete fMethod;
}

//_____________________________________________________________________________
const char* THaVar::GetTypeName( VarType itype )
{
  static const char* const type[] = { 
    "Double_t", "Float_t", "Long_t", "ULong_t", "Int_t", "UInt_t", "Short_t", 
    "UShort_t", "Char_t", "Byte_t", "TObject",
    "Double_t*", "Float_t*", "Long_t*", "ULong_t*", "Int_t*", "UInt_t*", 
    "Short_t*", "UShort_t*", "Char_t*", "Byte_t*", "TObject*",
    "Double_t**", "Float_t**", "Long_t**", "ULong_t**", "Int_t**", "UInt_t**", 
    "Short_t**", "UShort_t**", "Char_t**", "Byte_t**", "TObject**" };

  return type[itype];
}

//_____________________________________________________________________________
size_t THaVar::GetTypeSize( VarType itype )
{
  static const size_t size[] = { 
    sizeof(Double_t), sizeof(Float_t), sizeof(Long_t), sizeof(ULong_t), 
    sizeof(Int_t), sizeof(UInt_t), sizeof(Short_t), sizeof(UShort_t), 
    sizeof(Char_t), sizeof(Byte_t), 0, 
    sizeof(Double_t), sizeof(Float_t), sizeof(Long_t), sizeof(ULong_t), 
    sizeof(Int_t), sizeof(UInt_t), sizeof(Short_t), sizeof(UShort_t), 
    sizeof(Char_t), sizeof(Byte_t), 0,
    sizeof(Double_t), sizeof(Float_t), sizeof(Long_t), sizeof(ULong_t),
    sizeof(Int_t), sizeof(UInt_t), sizeof(Short_t), sizeof(UShort_t), 
    sizeof(Char_t), sizeof(Byte_t), 0  };

  return size[itype];
}

//_____________________________________________________________________________
Double_t THaVar::GetValueFromObject( Int_t i ) const
{
  // Retrieve variable from a ROOT object, either via a function call
  // or from a TSeqCollection

  if( fObject == NULL )
    return 0.0;

  void* obj;
  if( fOffset != -1 ) {
    // Array: Get data from the TSeqCollection

    // Get pointer to the object
    const TSeqCollection* c = static_cast<const TSeqCollection*>( fObject );
    obj = c->At(i);

    if( !fMethod ) {
      // No method ... get the data directly.
      // Compute location using the offset.
      ULong_t loc = (ULong_t)obj + fOffset;
      if( !loc || (fType >= kDoubleP && (*(void**)loc == NULL )) )
	return 0.0;
      switch( fType ) {
      case kDouble: 
	return *((Double_t*)loc);
      case kFloat:
	return static_cast<Double_t>( *(((Float_t*)loc))  );
      case kLong:
	return static_cast<Double_t>( *(((Long_t*)loc))   );
      case kULong:
	return static_cast<Double_t>( *(((ULong_t*)loc))  );
      case kInt:
	return static_cast<Double_t>( *(((Int_t*)loc))    );
      case kUInt:
	return static_cast<Double_t>( *(((UInt_t*)loc))   );
      case kShort:
	return static_cast<Double_t>( *(((Short_t*)loc))  );
      case kUShort:
	return static_cast<Double_t>( *(((UShort_t*)loc)) );
      case kChar:
	return static_cast<Double_t>( *(((Char_t*)loc))   );
      case kByte:
	return static_cast<Double_t>( *(((Byte_t*)loc))   );
      case kDoubleP: 
	return **((Double_t**)loc);
      case kFloatP:
	return static_cast<Double_t>( **(((Float_t**)loc))  );
      case kLongP:
	return static_cast<Double_t>( **(((Long_t**)loc))   );
      case kULongP:
	return static_cast<Double_t>( **(((ULong_t**)loc))  );
      case kIntP:
	return static_cast<Double_t>( **(((Int_t**)loc))    );
      case kUIntP:
	return static_cast<Double_t>( **(((UInt_t**)loc))   );
      case kShortP:
	return static_cast<Double_t>( **(((Short_t**)loc))  );
      case kUShortP:
	return static_cast<Double_t>( **(((UShort_t**)loc)) );
      case kCharP:
	return static_cast<Double_t>( **(((Char_t**)loc))   );
      case kByteP:
	return static_cast<Double_t>( **(((Byte_t**)loc))   );
      default:
	;
      }
      return 0.0;
    }

  } else {

    // No array, so it must be a function call. Everything else
    // is handled the standard way

    if( !fMethod )
      // Oops
      return 0.0;

    obj = const_cast<void*>(fObject);
  }

  if( fType != kDouble && fType != kFloat ) {
    // Integer data
    Long_t result;
    fMethod->Execute( obj, result );
    return static_cast<Double_t>( result );
  } else {
    // Floating-point data
    Double_t result;
    fMethod->Execute( obj, result );
    return result;
  }
}  

//_____________________________________________________________________________
Int_t THaVar::Index( const THaArrayString& elem ) const
{
  // Return linear index into this array variable corresponding 
  // to the array element described by 'elem'.  
  // Return -1 if subscript(s) out of bound(s) or -2 if incompatible arrays.

  if( elem.IsError() ) return -1;
  if( !elem.IsArray() ) return 0;

  if( fCount ) {
    if( elem.GetNdim() != 1 ) return -2;
    return *elem.GetDim();
  }

  Byte_t ndim = fArrayData.GetNdim();
  if( ndim != elem.GetNdim() ) return -2;

  const Int_t *subs = elem.GetDim(), *adim = fArrayData.GetDim();

  Int_t index = subs[0];
  for( Byte_t i = 0; i<ndim; i++ ) {
    if( subs[i]+1 > adim[i] ) return -1;
    if( i>0 )
      index = index*adim[i] + subs[i];
  }
  if( index >= fArrayData.GetLen() || index > kMaxInt ) return -1;
  return index;
}

//_____________________________________________________________________________
Int_t THaVar::Index( const char* s ) const
{
  // Return linear index into this array variable corresponding 
  // to the array element described by the string 's'.
  // 's' must be either a single integer subscript (for a 1-d array) 
  // or a comma-separated list of subscripts (for multi-dimensional arrays).
  //
  // NOTE: This method is vastly less efficient than 
  // THaVar::Index( THaArraySring& ) above because the string has 
  // to be parsed first.
  //
  // Return -1 if subscript(s) out of bound(s) or -2 if incompatible arrays.

  size_t len = strlen(s);
  if( len == 0 ) return 0;

  char* str = new char[ len+4 ];
  str[0] = 't';
  str[1] = '[';
  str[len+2] = ']';
  str[len+3] = '\0';
  strncpy( str+2, s, len );
  THaArrayString elem(str);
  delete [] str;

  return Index( elem );
}

//_____________________________________________________________________________
void THaVar::Print(Option_t* option) const
{
  //Print a description of this variable

  TNamed::Print(option);

  if( strcmp( option, "FULL" )) return;

  cout << "(" << GetTypeName() << ")[";
  if( fCount ) cout << "*";
  cout << GetLen() << "]";
  for( int i=0; i<GetLen(); i++ ) {
    cout << "  " << GetValue(i);
  }
  cout << endl;
}

//_____________________________________________________________________________
void THaVar::SetName( const char* name )
{
  // Set the name of the variable

  TNamed::SetName( name );
  fArrayData = name;
}

//_____________________________________________________________________________
void THaVar::SetNameTitle( const char* name, const char* descript )
{
  // Set name and description of the variable

  TNamed::SetNameTitle( name, descript );
  fArrayData = name;
}