// ProductRuntime.cpp
// Small facade so main.cpp remains an orchestrator instead of learning about
// every productization feature individually.

#ifndef IR_TRACKER_AMALGAMATED_BUILD
#error "Compile this module through main.cpp"
#endif

void beginProductRuntimeEarly(const esp_reset_reason_t reason) {
  beginProductSafety(reason);
}

void finishProductRuntimeSetup() {
  logProductSafetyBoot();
  setupProductExperienceRoutes();
}

void manageProductRuntime() {
  manageProductSafety();
  manageMeterCommissioning();
}

bool productRuntimeAllowsHistoryMigration() {
  return productHistoryMigrationAllowed();
}

bool productRuntimeAllowsAutomaticUpdate() {
  return productAutomaticUpdateAllowed();
}
