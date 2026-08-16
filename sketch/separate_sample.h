#include <vector>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <random>
#include <limits>
#include <iostream>
#include <algorithm>

template<uint32_t key_len>
struct Separate_sample {
    static constexpr int PURE_CM_LEVELS = 12;

    struct HHCell {
        uint32_t key = 0;
        int v = 0;
    };

    struct HybridLevel {
        std::vector<std::vector<HHCell>> hh;
        std::vector<std::vector<uint32_t>> cm;
        uint64_t cmPacketnum = 0;

        HybridLevel(int c, int l, int d, int m_small)
            : hh(c, std::vector<HHCell>(l)),
              cm(d, std::vector<uint32_t>(m_small, 0)) {}
    };

    struct Stats {
        double F2;
        double correctedF2;
        double entropy;
    };

    int d;
    int m;
    int m_small;
    int c;
    int l;

    double p_0;
    int sampleWeight;

    BOBHash32 **hash;
    BOBHash32 *hashx;

    uint64_t packetnum = 0;

    std::vector<std::vector<std::vector<uint32_t>>> counter;
    std::vector<HybridLevel> hybrid;

    std::mt19937 gen;
    std::uniform_real_distribution<double> dist;

    Separate_sample(
        int d_,
        int m_,
        int m_small_,
        int c_,
        int l_,
        double p_0_
    )
        : d(d_),
          m(m_),
          m_small(m_small_),
          c(c_),
          l(l_),
          p_0(p_0_),
          gen(std::random_device{}()),
          dist(0.0, 1.0) {

        static_assert(
            key_len >= 1 && key_len <= 32,
            "key_len must be in [1, 32]"
        );

        if (d <= 0 || m <= 1 || m_small <= 1 ||
            c <= 0 || l <= 0 || p_0 <= 0.0 || p_0 > 1.0) {
            throw std::invalid_argument("invalid parameters");
        }

        sampleWeight =
            static_cast<int>(std::round(1.0 / p_0));

        int pureLevels =
            std::min<int>(PURE_CM_LEVELS, key_len);

        counter.resize(
            pureLevels,
            std::vector<std::vector<uint32_t>>(
                d,
                std::vector<uint32_t>(m, 0)
            )
        );

        for (int i = PURE_CM_LEVELS; i < key_len; i++) {
            hybrid.emplace_back(c, l, d, m_small);
        }

        std::random_device rd;

        hash = new BOBHash32*[d];

        for (int i = 0; i < d; i++) {
            hash[i] =
                new BOBHash32(
                    uint8_t(rd() % MAX_PRIME32)
                );
        }

        hashx =
            new BOBHash32(
                uint8_t(rd() % MAX_PRIME32)
            );
    }

    ~Separate_sample() {
        for (int i = 0; i < d; i++) {
            delete hash[i];
        }

        delete[] hash;
        delete hashx;
    }

    Separate_sample(const Separate_sample&) = delete;
    Separate_sample& operator=(const Separate_sample&) = delete;


    void insertSmallCM(
        HybridLevel& module,
        uint32_t key,
        int weight
    ) {
        if (weight <= 0) {
            return;
        }

        for (int row = 0; row < d; row++) {
            uint32_t hashid =
                hash[row]->run((char*)&key, 4) % m_small;

            module.cm[row][hashid] += weight;
        }

        module.cmPacketnum += weight;
    }


    void insertHybrid(HybridLevel& module, uint32_t key) {
        uint32_t bucketID = hashx->run((char*)&key, 4) % c;

        auto& bucket = module.hh[bucketID];

        HHCell* emptyCell = nullptr;
        HHCell* minCell = &bucket[0];

        for (auto& cell : bucket) {
            if (cell.v > 0 && cell.key == key) {
                cell.v += sampleWeight;
                return;
            }

            if (cell.v == 0 && emptyCell == nullptr) {
                emptyCell = &cell;
            }

            if (cell.v < minCell->v) {
                minCell = &cell;
            }
        }

        if (emptyCell != nullptr) {
            emptyCell->key = key;
            emptyCell->v = sampleWeight;
            return;
        }

        if (sampleWeight < minCell->v) {
            minCell->v -= sampleWeight;
            insertSmallCM(module, key, sampleWeight);
            return;
        }

        int cmWeight = minCell->v - 1;
        int newValue = sampleWeight - minCell->v + 1;

        insertSmallCM(module, key, cmWeight);

        minCell->key = key;
        minCell->v = newValue;
    }


    void insert(
        const std::vector<uint32_t>& key_list,
        int levels
    ) {
        if (levels <= 0 ||
            levels > static_cast<int>(key_len) ||
            key_list.size() < static_cast<size_t>(levels)) {
            throw std::invalid_argument("invalid levels");
        }

        packetnum++;

        if (dist(gen) >= p_0) {
            return;
        }

        for (int level = 0; level < levels; level++) {
            uint32_t key = key_list[level];

            if (level < PURE_CM_LEVELS) {
                for (int row = 0; row < d; row++) {
                    uint32_t hashid =
                        hash[row]->run((char*)&key, 4) % m;

                    counter[level][row][hashid] += sampleWeight;
                }
            }
            else {
                insertHybrid(
                    hybrid[level - PURE_CM_LEVELS],
                    key
                );
            }
        }
    }


    Stats estimateF2Entropy(
        int sketchID,
        double total
    ) const {
        double minF2 =
            std::numeric_limits<double>::infinity();

        double minRawEntropy =
            std::numeric_limits<double>::infinity();

        for (int row = 0; row < d; row++) {
            double rowF2 = 0.0;
            double rawEntropy = 0.0;

            for (int col = 0; col < m; col++) {
                double cnt =
                    static_cast<double>(
                        counter[sketchID][row][col]
                    );

                rowF2 += cnt * cnt;

                if (cnt > 0.0) {
                    rawEntropy +=
                        cnt * std::log2(cnt);
                }
            }

            if (rowF2 < minF2) {
                minF2 = rowF2;
            }

            if (rawEntropy < minRawEntropy) {
                minRawEntropy = rawEntropy;
            }
        }

        double correctedF2 =
            (m * minF2 - total * total) /
            (m - 1.0);

        double entropy =
            std::log2(total) -
            minRawEntropy / total;

        return {
            minF2,
            correctedF2,
            entropy
        };
    }


    Stats estimateHybrid(int hybridID, double total) const {
        const auto& module = hybrid[hybridID];

        double hhF2 = 0.0;
        double hhRawEntropy = 0.0;

        for (const auto& bucket : module.hh) {
            for (const auto& cell : bucket) {
                if (cell.v > 0) {
                    double cnt = static_cast<double>(cell.v);
                    hhF2 += cnt * cnt;
                    hhRawEntropy += cnt * std::log2(cnt);
                }
            }
        }

        double cmF2 = 0.0;
        double cmRawEntropy = 0.0;

        if (module.cmPacketnum > 0) {
            cmF2 = std::numeric_limits<double>::infinity();
            cmRawEntropy = std::numeric_limits<double>::infinity();

            for (int row = 0; row < d; row++) {
                double rowF2 = 0.0;
                double rawEntropy = 0.0;

                for (int col = 0; col < m_small; col++) {
                    double cnt = static_cast<double>(module.cm[row][col]);
                    rowF2 += cnt * cnt;

                    if (cnt > 0.0) {
                        rawEntropy += cnt * std::log2(cnt);
                    }
                }

                if (rowF2 < cmF2) {
                    cmF2 = rowF2;
                }

                if (rawEntropy < cmRawEntropy) {
                    cmRawEntropy = rawEntropy;
                }
            }
        }

        double correctedCMF2 = cmF2;

        if (module.cmPacketnum > 0) {
            double cmTotal = static_cast<double>(module.cmPacketnum);
            correctedCMF2 = (m_small * cmF2 - cmTotal * cmTotal) / (m_small - 1.0);
        }

        double F2 = hhF2 + cmF2;
        double correctedF2 = hhF2 + correctedCMF2;
        double entropy = std::log2(total) - (hhRawEntropy + cmRawEntropy) / total;

        return {F2, correctedF2, entropy};
    }


    std::vector<double> query(
        int levels
    ) const {
        if (packetnum == 0) {
            throw std::runtime_error(
                "Separate is empty"
            );
        }

        double total =
            static_cast<double>(
                packetnum
            );

        std::vector<double> F2_x;
        std::vector<double> F2_y;

        std::vector<double> Var_x;
        std::vector<double> Var_y;

        std::vector<double> Ent_x;
        std::vector<double> Ent_y;

        F2_x.reserve(levels + 1);
        F2_y.reserve(levels + 1);

        Var_x.reserve(levels);
        Var_y.reserve(levels);

        Ent_x.reserve(levels + 1);
        Ent_y.reserve(levels + 1);

        F2_x.push_back(0.0);
        F2_y.push_back(
            std::log2(total * total)
        );

        Ent_x.push_back(0.0);
        Ent_y.push_back(0.0);


        for (int level = 1; level <= levels; level++) {
            Stats stats;

            if (level <= PURE_CM_LEVELS) {
                stats =
                    estimateF2Entropy(
                        level - 1,
                        total
                    );
            }
            else {
                stats =
                    estimateHybrid(
                        level - PURE_CM_LEVELS - 1,
                        total
                    );
            }

            double F2 =
                stats.F2;

            double correctedF2 =
                stats.correctedF2;

            double entropy =
                stats.entropy;


            F2_x.push_back(
                static_cast<double>(level)
            );

            F2_y.push_back(
                std::log2(correctedF2)
            );


            Ent_x.push_back(
                static_cast<double>(level)
            );

            Ent_y.push_back(
                entropy
            );


            double nodeNum =
                std::exp2(
                    static_cast<double>(level)
                );

            double variance =
                nodeNum * F2 /
                (total * total) -
                1.0;


            if (variance > 0.0) {
                Var_x.push_back(
                    -static_cast<double>(level)
                );

                Var_y.push_back(
                    std::log2(variance)
                );
            }


            std::cout
                << "p = " << level
                << " Fractal = "
                << std::log2(F2)
                << " CorrectedFractal = "
                << std::log2(correctedF2)
                << " Variance = "
                << std::log2(variance)
                << " Entropy = "
                << entropy
                << std::endl;
        }


        double F2Slope =
            getRatio(
                F2_x,
                F2_y
            );

        double varSlope =
            getRatio(
                Var_x,
                Var_y
            );

        double entSlope =
            getRatio(
                Ent_x,
                Ent_y
            );


        double bF2 =
            getBFromFractal(
                F2Slope
            );

        double bVariance =
            getBFromVariance(
                varSlope
            );

        double bEntropy =
            getBFromEntropy(
                entSlope
            );


        return {
            bF2,
            bVariance,
            bEntropy
        };
    }
};