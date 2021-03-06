#include "B4DetectorConstruction.hh"

#include "G4Material.hh"
#include "G4Element.hh"
#include "G4NistManager.hh"

#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4PVReplica.hh"
#include "G4GlobalMagFieldMessenger.hh"
#include "G4AutoDelete.hh"

#include "G4GeometryManager.hh"
#include "G4PhysicalVolumeStore.hh"
#include "G4LogicalVolumeStore.hh"
#include "G4SolidStore.hh"

#include "G4VisAttributes.hh"
#include "G4Colour.hh"

#include "G4PhysicalConstants.hh"
#include "G4SystemOfUnits.hh"

#include "G4Polyhedra.hh"
#include "G4NistManager.hh"
#include "G4Trd.hh"

G4ThreadLocal
G4GlobalMagFieldMessenger* B4DetectorConstruction::fMagFieldMessenger = 0;

B4DetectorConstruction::B4DetectorConstruction(std::vector<Scintillator> scints)
 : G4VUserDetectorConstruction(),
   mScints(scints),
   fCheckOverlaps(true)
{
}

B4DetectorConstruction::~B4DetectorConstruction()
{
}

G4VPhysicalVolume* B4DetectorConstruction::Construct()
{
  // Define materials
  DefineMaterials();

  // Define volumes
  return DefineVolumes();
}

void B4DetectorConstruction::DefineMaterials()
{

  G4NistManager* nistManager = G4NistManager::Instance();

  // Liquid argon material
  G4double a;  // mass of a mole;
  G4double z;  // z=mean number of protons;
  G4double density;
  new G4Material("liquidArgon", z=18., a= 39.95*g/mole, density= 1.390*g/cm3);
         // The argon by NIST Manager is a gas with a different density


  // Vacuum
  new G4Material("Galactic", z=1., a=1.01*g/mole,density= universe_mean_density,
                  kStateGas, 2.73*kelvin, 3.e-18*pascal);


  // Print materials
  G4cout << *(G4Material::GetMaterialTable()) << G4endl;
}

G4VPhysicalVolume* B4DetectorConstruction::makeTriangle(G4LogicalVolume *worldLV, G4ThreeVector pos,
		G4RotationMatrix *rot, G4String name, G4double sideLength, G4double depth){
	G4RotationMatrix *finalRot;
	if (rot){
		finalRot = new G4RotationMatrix(*rot);//NOTE: this allocates memory that is never freed.
	} else {
		finalRot = new G4RotationMatrix();
	}
	finalRot->rotateX(pi/2);
	finalRot->rotateZ(pi/2);
	float heightWidthRatio =  .86602540;//This is the ratio of height to width in an equilateral triangle.
	G4NistManager *nist = G4NistManager::Instance();
//	G4Material* polystyrene = nist->FindOrBuildMaterial("G4_Pb");
    G4Material* polystyrene = nist->FindOrBuildMaterial("G4_POLYSTYRENE");
	//NOTE: y direction is depth, x direction is width.
	//TO put that another way, a plane perpendicular to y axis cuts a triangle cross section
	//The pointy edge points in the positive z direction
	G4VSolid *triangly = new G4Trd(name,
            /*dx1*/sideLength / 2,
            /*dx2*/0,
            /*dy1*/depth / 2,
            /*dy2*/depth / 2,
            /*dz*/sideLength * heightWidthRatio / 2);

    G4LogicalVolume* trianglyLV = new G4LogicalVolume(triangly, polystyrene, "Triangly");
    G4PVPlacement* trianglyPV = new G4PVPlacement(
  	finalRot, //yes rotation
  	pos,
  	trianglyLV,
  	"Triangly",
  	worldLV, //place it in the world
  	false, // no boolean operation (whatever that means)
  	0, //copy number apparently
  	true);//better check for those overlaps

    //tell the compile I don't care this variable wasn't used.
    //Note that really all these things should be deleted at some point.
    return  trianglyPV;
}

G4VPhysicalVolume* B4DetectorConstruction::DefineVolumes()
{
  // Geometry parameters

  G4double worldSizeXY = 50 * cm;
  G4double worldSizeZ  = 50 * cm;
//  G4double worldSizeXY = 10 * m;
//  G4double worldSizeZ  = 10 * m;

  G4Material* defaultMaterial = G4Material::GetMaterial("Galactic");

  G4VSolid* worldS
    = new G4Box("World",           // its name
                 worldSizeXY/2, worldSizeXY/2, worldSizeZ/2); // its size

  G4LogicalVolume* worldLV
    = new G4LogicalVolume(
                 worldS,           // its solid
                 defaultMaterial,  // its material
                 "World");         // its name

  G4VPhysicalVolume* worldPV
    = new G4PVPlacement(
                 0,                // no rotation
                 G4ThreeVector(),  // at (0,0,0)
                 worldLV,          // its logical volume
                 "World",          // its name
                 0,                // its mother  volume
                 false,            // no boolean operation
                 0,                // copy number
                 fCheckOverlaps);  // checking overlaps

  /*Make triangle geometry out of Scintillator description*/

  for (size_t i = 0; i < mScints.size(); i++){
	  scintVolumes.push_back(makeTriangle(worldLV, mScints[i].getPosition(), NULL, "temp-name",
			  10 * cm, 20*cm));
  }

  worldLV->SetVisAttributes (G4VisAttributes::Invisible);

  G4VisAttributes* simpleBoxVisAtt= new G4VisAttributes(G4Colour(1.0,1.0,1.0));
  simpleBoxVisAtt->SetVisibility(true);
//  calorLV->SetVisAttributes(simpleBoxVisAtt);

  //
  // Always return the physical World
  //
  return worldPV;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void B4DetectorConstruction::ConstructSDandField()
{
  // Create global magnetic field messenger.
  // Uniform magnetic field is then created automatically if
  // the field value is not zero.
  G4ThreeVector fieldValue = G4ThreeVector();
  fMagFieldMessenger = new G4GlobalMagFieldMessenger(fieldValue);
  fMagFieldMessenger->SetVerboseLevel(1);

  // Register the field messenger for deleting
  G4AutoDelete::Register(fMagFieldMessenger);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

//FUN FACT: rotate viewer be like: /vis/viewer/set/viewpointThetaPhi 100 50
