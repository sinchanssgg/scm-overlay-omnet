#ifndef SCMNODE_H
#define SCMNODE_H

#include <omnetpp.h>
#include "SCMMessages.h"
#include <vector>
#include <string>

// Suppress OpenSSL 3.0 deprecation warnings for EC_KEY API
#define OPENSSL_SUPPRESS_DEPRECATED
#include <openssl/ec.h>

using namespace omnetpp;

// Proof structure for cryptographic auditing — declared before SCMNode
// because SCMNode methods reference it.
struct ProofStruct {
    int nodeId;
    int numUsers;
    int subtreeSize;
    double betaValue;
    double payment;
    std::vector<uint8_t> parentBetaSig;
    std::vector<std::vector<uint8_t>> childrenSigs;
    std::vector<std::vector<uint8_t>> childProofs;

    ProofStruct() : nodeId(-1), numUsers(0), subtreeSize(0),
                   betaValue(0.0), payment(0.0) {}
};

class SCMNode : public omnetpp::cSimpleModule {
  private:
    enum class AlgorithmKind { SCM, GARG_GROSU, BYRENHEID };

    // Node state variables
    int id;
    int parentId;
    int level;
    enum Status { STABLE, FAULTY, RECOVERING } status;
    double payment;
    int subtreeSize;
    double beta;
    int numUsers;
    double linkCost;
    AlgorithmKind algorithmKind;
    simsignal_t stabilizationTimeSignal;
    double lastFaultTime;

    // Garg-Grosu convergence detection (compare beta across consecutive rounds)
    double prevBeta;
    bool ggConverged;
    int roundCounter;

    // Cryptographic state variables
    EC_KEY *eckey;  // Elliptic curve key pair
    std::vector<uint8_t> sizeSig;  // Signature of subtreeSize
    std::vector<uint8_t> betaSig;  // Signature of beta
    std::vector<uint8_t> proof;    // Cryptographic proof for auditing

    // Helper methods for stabilization rules
    bool notLocallyConsistent();
    bool lostStableSupport();
    bool allChildrenRecovering();
    bool allChildrenHaveProofs();
    bool existsBetterParent();
    int findBestParent();
    double parentScore(const SCMNode* candidate) const;
    void calculateAlpha();
    void calculateBeta();
    void notifyChildren(SCMControlMessage::MsgType msgType);

    // Cryptographic helper methods
    std::vector<uint8_t> signMessage(const std::string& message);
    bool verifySignature(const std::string& message,
                        const std::vector<uint8_t>& signature,
                        EC_KEY* key);
    std::string serializeProof(const ProofStruct& proofData);
    ProofStruct deserializeProof(const std::string& proofStr);

    // Phase-specific handlers
    void handleStabilization();
    void handleAlphaUpdate(SCMControlMessage* msg);
    void handleBetaUpdate(SCMControlMessage* msg);
    void handleFaultNotification(SCMControlMessage* msg);
    void handleProofRequest(SCMControlMessage* msg);
    void handleProofResponse(SCMControlMessage* msg);

    // Recovery phase methods
    bool rejoinTree();

    // State publication phase methods
    void signState();

    // Proof propagation phase methods
    void buildProof();
    void verifyProofChain();
    bool verifyNodeProof(SCMNode* node);
    void handleCheatingNode(int cheatingNodeId);

    // Utility methods
    double calculateMaxGain();
    SCMNode* getParentNode();
    std::vector<SCMNode*> getChildrenNodes();
    SCMNode* getNodeById(int nodeId);
    static AlgorithmKind parseAlgorithmKind(const char* variant);
    static const char* algorithmKindLabel(AlgorithmKind kind);

  protected:
    virtual void initialize() override;
    virtual void handleMessage(omnetpp::cMessage *msg) override;
    virtual void refreshDisplay() const override;
    virtual void finish() override;

  public:
    SCMNode();
    virtual ~SCMNode();

    // Getters for fault injection and monitoring
    int getId() const { return id; }
    int getParentId() const { return parentId; }
    int getLevel() const { return level; }
    double getBeta() const { return beta; }
    const char* getStatusLabel() const {
        return (status == STABLE) ? "STABLE" : (status == FAULTY) ? "FAULTY" : "RECOVERING";
    }
    Status getStatus() const { return status; }
    double getPayment() const { return payment; }
    int getSubtreeSize() const { return subtreeSize; }
    const std::vector<uint8_t>& getProof() const { return proof; }

    // Setters for fault injection
    void setLevel(int lvl) { level = lvl; }
    void setBeta(double b) { beta = b; }
    void setParentId(int pid) { parentId = pid; }
    void setStatus(Status s) { status = s; }
    void setSubtreeSize(int size) { subtreeSize = size; }
    void corruptProof() { proof.clear(); }
};

#endif // SCMNODE_H
