#include <vector>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <random>
#include <limits>
#include <iostream>
#include <algorithm>

template<uint32_t key_len>
struct Separate_sample_super {
    static constexpr int PURE_CM_LEVELS = 12;

    struct HHCell {
        uint32_t key = 0;
        int v = 0;
    };

    struct HHLevel {
        std::vector<std::vector<HHCell>> hh;
        uint64_t hhPacketnum = 0;

        HHLevel(int c, int l) : hh(c, std::vector<HHCell>(l)) {}
    };

    struct Stats {
        double F2;
        double correctedF2;
        double entropy;
    };

    int d;
    int m;
    int c;
    int l;

    double p_0;
    int sampleWeight;

    BOBHash32 **hash;
    BOBHash32 *hashx;

    uint64_t packetnum = 0;

    std::vector<std::vector<std::vector<uint32_t>>> counter;
    std::vector<HHLevel> hhLevels;

    std::mt19937 gen;
    std::uniform_real_distribution<double> dist;

    Separate_sample_super(int d_, int m_, int c_, int l_, double p_0_)
        : d(d_), m(m_), c(c_), l(l_), p_0(p_0_), gen(std::random_device{}()), dist(0.0, 1.0) {

        static_assert(key_len >= 1 && key_len <= 32, "key_len must be in [1, 32]");

        if (d <= 0 || m <= 1 || c <= 0 || l <= 0 || p_0 <= 0.0 || p_0 > 1.0) {
            throw std::invalid_argument("invalid parameters");
        }

        sampleWeight = static_cast<int>(std::round(1.0 / p_0));

        int pureLevels = std::min<int>(PURE_CM_LEVELS, key_len);

        counter.resize(pureLevels, std::vector<std::vector<uint32_t>>(d, std::vector<uint32_t>(m, 0)));

        for (int i = PURE_CM_LEVELS; i < key_len; i++) {
            hhLevels.emplace_back(c, l);
        }

        std::random_device rd;

        hash = new BOBHash32*[d];

        for (int i = 0; i < d; i++) {
            hash[i] = new BOBHash32(uint8_t(rd() % MAX_PRIME32));
        }

        hashx = new BOBHash32(uint8_t(rd() % MAX_PRIME32));
    }

    ~Separate_sample_super() {
        for (int i = 0; i < d; i++) {
            delete hash[i];
        }

        delete[] hash;
        delete hashx;
    }

    Separate_sample_super(const Separate_sample_super&) = delete;
    Separate_sample_super& operator=(const Separate_sample_super&) = delete;


    void insertHH(HHLevel& module, uint32_t key) {
        uint32_t bucketID = hashx->run((char*)&key, 4) % c;
        auto& bucket = module.hh[bucketID];

        HHCell* emptyCell = nullptr;
        HHCell* minCell = &bucket[0];

        for (auto& cell : bucket) {
            if (cell.v > 0 && cell.key == key) {
                cell.v+=sampleWeight;
                module.hhPacketnum += sampleWeight;
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
            module.hhPacketnum += sampleWeight;
            return;
        }

        minCell->v--;

        if (minCell->v == 0) {
            minCell->key = key;
            minCell->v = 1;
            return;
        }
        // double replaceProb = 1.0 / (static_cast<double>(minCell->v) + 1.0);
        // double randomValue = static_cast<double>(std::rand()) / static_cast<double>(RAND_MAX);
        //
        // if (randomValue < replaceProb) {
        //     minCell->key = key;
        //     minCell->v+=sampleWeight;
        //     return;
        // }

    }


    void insert(const std::vector<uint32_t>& key_list, int levels) {
        if (levels <= 0 || levels > static_cast<int>(key_len) || key_list.size() < static_cast<size_t>(levels)) {
            throw std::invalid_argument("invalid levels");
        }

        packetnum++;

        double r = dist(gen);

        if (r >= p_0) {
            return;
        }

        for (int level = 0; level < levels; level++) {
            uint32_t key = key_list[level];

            if (level < PURE_CM_LEVELS) {
                for (int row = 0; row < d; row++) {
                    uint32_t hashid = hash[row]->run((char*)&key, 4) % m;
                    counter[level][row][hashid] += sampleWeight;
                }
            }
            else {
                insertHH(hhLevels[level - PURE_CM_LEVELS], key);
            }
        }
    }


    Stats estimateF2Entropy(int sketchID, double total) const {
        double minF2 = std::numeric_limits<double>::infinity();
        double minRawEntropy = std::numeric_limits<double>::infinity();

        for (int row = 0; row < d; row++) {
            double rowF2 = 0.0;
            double rawEntropy = 0.0;

            for (int col = 0; col < m; col++) {
                double cnt = static_cast<double>(counter[sketchID][row][col]);

                rowF2 += cnt * cnt;

                if (cnt > 0.0) {
                    rawEntropy += cnt * std::log2(cnt);
                }
            }

            if (rowF2 < minF2) {
                minF2 = rowF2;
            }

            if (rawEntropy < minRawEntropy) {
                minRawEntropy = rawEntropy;
            }
        }

        double correctedF2 = (m * minF2 - total * total) / (m - 1.0);
        double entropy = std::log2(total) - minRawEntropy / total;

        return {minF2, correctedF2, entropy};
    }


    Stats estimateHH(int hhID, int level, double total) const {
        const auto& module = hhLevels[hhID];

        double F2 = 0.0;
        double rawEntropy = 0.0;
        double sum = 0;
        for (const auto& bucket : module.hh) {
            for (const auto& cell : bucket) {
                if (cell.v > 0) {
                    double cnt = static_cast<double>(cell.v);
                    sum += cnt;
                    F2 += cnt * cnt;
                    rawEntropy += cnt * std::log2(cnt);
                }
            }
        }

        double nodeNum = std::exp2(static_cast<double>(level));
        double otherNodeNum = nodeNum - static_cast<double>(c * l);
        // double remaining = total - static_cast<double>(module.hhPacketnum);
        double remaining = total - sum;
        if (remaining > 0.0 && otherNodeNum > 0.0) {
            if (remaining < otherNodeNum) {
                F2 += remaining;
            } else {
                double avg = remaining / otherNodeNum;
                F2 += otherNodeNum * avg * avg;
                rawEntropy += otherNodeNum * avg * std::log2(avg);
            }
        }

        double entropy = std::log2(total) - rawEntropy / total;

        return {F2, F2, entropy};
    }


    double queryHHFrequency(int level, uint32_t key) const {
        if (level <= PURE_CM_LEVELS || level > static_cast<int>(key_len)) {
            throw std::invalid_argument("invalid HH level");
        }

        const auto& module = hhLevels[level - PURE_CM_LEVELS - 1];

        uint32_t bucketID = hashx->run((char*)&key, 4) % c;
        const auto& bucket = module.hh[bucketID];

        for (const auto& cell : bucket) {
            if (cell.v > 0 && cell.key == key) {
                return static_cast<double>(cell.v);
            }
        }

        double nodeNum = std::exp2(static_cast<double>(level));
        double otherNodeNum = nodeNum - static_cast<double>(c * l);
        double remaining = std::max(0.0, static_cast<double>(packetnum) - static_cast<double>(module.hhPacketnum));

        if (otherNodeNum <= 0.0) {
            return 0.0;
        }

        return remaining / otherNodeNum;
    }


    std::vector<double> query(int levels) const {
        if (packetnum == 0) {
            throw std::runtime_error("Separate_sample_super is empty");
        }

        if (levels <= 0 || levels > static_cast<int>(key_len)) {
            throw std::invalid_argument("invalid levels");
        }

        double total = static_cast<double>(packetnum);

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
        F2_y.push_back(std::log2(total * total));

        Ent_x.push_back(0.0);
        Ent_y.push_back(0.0);

        for (int level = 1; level <= levels; level++) {
            Stats stats;

            if (level <= PURE_CM_LEVELS) {
                stats = estimateF2Entropy(level - 1, total);
            }
            else {
                stats = estimateHH(level - PURE_CM_LEVELS - 1, level, total);
            }

            double F2 = stats.F2;
            double correctedF2 = stats.correctedF2;
            double entropy = stats.entropy;

            F2_x.push_back(static_cast<double>(level));
            F2_y.push_back(std::log2(correctedF2));

            Ent_x.push_back(static_cast<double>(level));
            Ent_y.push_back(entropy);

            double nodeNum = std::exp2(static_cast<double>(level));
            double variance = nodeNum * F2 / (total * total) - 1.0;

            if (variance > 0.0) {
                Var_x.push_back(-static_cast<double>(level));
                Var_y.push_back(std::log2(variance));
            }

            std::cout << "p = " << level
                      << " Fractal = " << std::log2(F2)
                      << " CorrectedFractal = " << std::log2(correctedF2)
                      << " Variance = " << std::log2(variance)
                      << " Entropy = " << entropy
                      << std::endl;
        }

        double F2Slope = getRatio(F2_x, F2_y);
        double varSlope = getRatio(Var_x, Var_y);
        double entSlope = getRatio(Ent_x, Ent_y);

        double bF2 = getBFromFractal(F2Slope);
        double bVariance = getBFromVariance(varSlope);
        double bEntropy = getBFromEntropy(entSlope);

        return {bF2, bVariance, bEntropy};
    }
};