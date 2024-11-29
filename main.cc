#include "G4Box.hh"
#include "G4Cons.hh"
#include "G4LogicalVolume.hh"
#include "G4NistManager.hh"
#include "G4PVPlacement.hh"
#include "G4ParticleGun.hh"
#include "G4RunManager.hh"
#include "G4RunManagerFactory.hh"
#include "G4SteppingVerbose.hh"
#include "G4SystemOfUnits.hh"
#include "G4UIExecutive.hh"
#include "G4UImanager.hh"
#include "G4UserEventAction.hh"
#include "G4UserRunAction.hh"
#include "G4VUserActionInitialization.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4VisExecutive.hh"
#include "QBBC.hh"

class RunAction final : public G4UserRunAction {
public:
  RunAction()           = default;
  ~RunAction() override = default;

  void BeginOfRunAction(const G4Run*) override {
    // inform the runManager to save random number seed
    G4RunManager::GetRunManager()->SetRandomNumberStore(false);
  }
  void EndOfRunAction(const G4Run*) override {}
};

class EventAction final : public G4UserEventAction {
public:
  explicit EventAction(RunAction* runAction) : fRunAction(runAction) {}
  ~EventAction() override = default;

  void BeginOfEventAction(const G4Event* event) override {}
  void EndOfEventAction(const G4Event* event) override {}

private:
  RunAction* fRunAction = nullptr;
};

class SteppingAction final : public G4UserSteppingAction {
public:
  explicit SteppingAction(EventAction* eventAction)
      : fEventAction(eventAction) {}
  ~SteppingAction() override = default;

  // method from the base class
  void UserSteppingAction(const G4Step*) override {}

private:
  EventAction* fEventAction = nullptr;
};

class DetectorConstruction final : public G4VUserDetectorConstruction {
public:
  DetectorConstruction()           = default;
  ~DetectorConstruction() override = default;

  G4VPhysicalVolume* Construct() override {
    // Get nist material manager
    G4NistManager* nist = G4NistManager::Instance();

    // Envelope parameters
    //
    G4double    env_sizeXY = 20 * cm, env_sizeZ = 30 * cm;
    G4Material* env_mat = nist->FindOrBuildMaterial("G4_WATER");

    // Option to switch on/off checking of volumes overlaps
    //
    G4bool checkOverlaps = true;

    //
    // World
    //
    G4double    world_sizeXY = 1.2 * env_sizeXY;
    G4double    world_sizeZ  = 1.2 * env_sizeZ;
    G4Material* world_mat    = nist->FindOrBuildMaterial("G4_AIR");

    auto solidWorld = new G4Box("World", // its name
                                0.5 * world_sizeXY, 0.5 * world_sizeXY,
                                0.5 * world_sizeZ); // its size

    auto logicWorld = new G4LogicalVolume(solidWorld, // its solid
                                          world_mat,  // its material
                                          "World");   // its name

    auto physWorld = new G4PVPlacement(nullptr,         // no rotation
                                       G4ThreeVector(), // at (0,0,0)
                                       logicWorld,      // its logical volume
                                       "World",         // its name
                                       nullptr,         // its mother  volume
                                       false,           // no boolean operation
                                       0,               // copy number
                                       checkOverlaps);  // overlaps checking
    return physWorld;
  }
};

class PrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction {
public:
  PrimaryGeneratorAction() {
    constexpr G4int n_particle = 1;
    fParticleGun               = new G4ParticleGun(n_particle);

    // default particle kinematic
    G4ParticleTable*      particleTable = G4ParticleTable::GetParticleTable();
    G4String              particleName;
    G4ParticleDefinition* particle
        = particleTable->FindParticle(particleName = "gamma");
    fParticleGun->SetParticleDefinition(particle);
    fParticleGun->SetParticleMomentumDirection(G4ThreeVector(0., 0., 1.));
    fParticleGun->SetParticleEnergy(6. * MeV);
  }
  ~PrimaryGeneratorAction() override { delete fParticleGun; }

  // method from the base class
  void GeneratePrimaries(G4Event* anEvent) override {
    fParticleGun->SetParticlePosition(G4ThreeVector(0., 0., 0.));

    fParticleGun->GeneratePrimaryVertex(anEvent);
  }

private:
  G4ParticleGun* fParticleGun = nullptr; // pointer a to G4 gun class
};

class ActionInitialization final : public G4VUserActionInitialization {
public:
  ActionInitialization()           = default;
  ~ActionInitialization() override = default;

  void BuildForMaster() const override {
    auto runAction = new RunAction;
    SetUserAction(runAction);
  }
  void Build() const override {
    SetUserAction(new PrimaryGeneratorAction);

    auto runAction = new RunAction;
    SetUserAction(runAction);

    auto eventAction = new EventAction(runAction);
    SetUserAction(eventAction);

    SetUserAction(new SteppingAction(eventAction));
  }
};

int main(int argc, char* argv[]) {
  // Detect interactive mode (if no arguments) and define UI session
  //
  G4UIExecutive* ui = nullptr;
  if (argc == 1) { ui = new G4UIExecutive(argc, argv); }

  // Optionally: choose a different Random engine...
  // G4Random::setTheEngine(new CLHEP::MTwistEngine);

  // use G4SteppingVerboseWithUnits
  G4int precision = 4;
  G4SteppingVerbose::UseBestUnit(precision);

  // Construct the default run manager
  //
  auto runManager
      = G4RunManagerFactory::CreateRunManager(G4RunManagerType::Default);

  // Set mandatory initialization classes
  //
  // Detector construction
  runManager->SetUserInitialization(new DetectorConstruction());

  // Physics list
  auto physicsList = new QBBC;
  physicsList->SetVerboseLevel(1);
  runManager->SetUserInitialization(physicsList);

  // User action initialization
  runManager->SetUserInitialization(new ActionInitialization());

  // Initialize visualization with the default graphics system
  auto visManager = new G4VisExecutive(argc, argv);
  // Constructors can also take optional arguments:
  // - a graphics system of choice, eg. "OGL"
  // - and a verbosity argument - see /vis/verbose guidance.
  // auto visManager = new G4VisExecutive(argc, argv, "OGL", "Quiet");
  // auto visManager = new G4VisExecutive("Quiet");
  visManager->Initialize();

  // Get the pointer to the User Interface manager
  auto UImanager = G4UImanager::GetUIpointer();

  // Process macro or start UI session
  //
  if (! ui) {
    // batch mode
    G4String command  = "/control/execute ";
    G4String fileName = argv[1];
    UImanager->ApplyCommand(command + fileName);
  } else {
    // interactive mode
    UImanager->ApplyCommand("/control/execute init_vis.mac");
    ui->SessionStart();
    delete ui;
  }

  // Job termination
  // Free the store: user actions, physics_list and detector_description are
  // owned and deleted by the run manager, so they should not be deleted
  // in the main() program !

  delete visManager;
  delete runManager;
  return 0;
}
