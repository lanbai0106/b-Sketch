#include <vector>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <random>
#include <limits>

template<uint32_t key_len>
struct CM_sample {
    int d;
    int m;
    double p_0;
    uint32_t sampleWeight;

    BOBHash32 **hash;

    uint32_t packetnum;

    std::vector<uint32_t> levelPacketnum;
    std::vector<std::vector<std::vector<uint32_t>>> counter;

    mutable std::mt19937 gen;
    mutable std::uniform_real_distribution<double> dist;


    CM_sample(int d_, int m_, double p_0_)
        : d(d_),
          m(m_),
          p_0(p_0_),
          packetnum(0),
          gen(std::random_device{}()),
          dist(0.0, 1.0) {

        static_assert(
            key_len >= 1 && key_len <= 32,
            "key_len must be in [1, 32]"
        );

        if (d <= 0 || m <= 0) {
            throw std::invalid_argument(
                "d and m must be positive"
            );
        }

        if (p_0 <= 0.0 || p_0 > 1.0) {
            throw std::invalid_argument(
                "p_0 must be in (0, 1]"
            );
        }

        sampleWeight =
            static_cast<uint32_t>(
                std::round(1.0 / p_0)
            );

        counter.resize(
            key_len,
            std::vector<std::vector<uint32_t>>(
                d,
                std::vector<uint32_t>(m, 0)
            )
        );

        levelPacketnum.resize(
            key_len,
            0
        );

        hash =
            new BOBHash32*[d];

        std::random_device rd;

        for (int i = 0; i < d; i++) {
            hash[i] =
                new BOBHash32(
                    uint8_t(
                        rd() % MAX_PRIME32
                    )
                );
        }
    }


    ~CM_sample() {
        for (int i = 0; i < d; i++) {
            delete hash[i];
        }

        delete[] hash;
    }


    void insert(
        const std::vector<uint32_t>& key_list,
        double p
    ) {
        int levels = p;

        if (
            key_list.size() <
            static_cast<size_t>(levels)
        ) {
            throw std::invalid_argument(
                "key_list does not contain enough prefix keys"
            );
        }

        packetnum++;

        for (int level = 0; level < levels; level++) {
            levelPacketnum[level]++;
        }

        double r = dist(gen);

        if (r >= p_0) {
            return;
        }

        for (int level = 0; level < levels; level++) {
            uint32_t key =
                key_list[level];

            for (int row = 0; row < d; row++) {
                uint32_t hashid =
                    hash[row]->run(
                        (char *)&key,
                        4
                    ) % m;

                counter[level][row][hashid]
                    += sampleWeight;
            }
        }
    }


    double estimateF2(int sketchID) const {
        double minF2 =
            std::numeric_limits<double>::infinity();

        for (int row = 0; row < d; row++) {
            double rowF2 = 0.0;

            for (int col = 0; col < m; col++) {
                double cnt =
                    static_cast<double>(
                        counter[sketchID][row][col]
                    );

                rowF2 += cnt * cnt;
            }

            if (rowF2 < minF2) {
                minF2 = rowF2;
            }
        }

        return minF2;
    }


    double estimateEntropy(
        int sketchID,
        double total
    ) const {
        double minRawEntropy =
            std::numeric_limits<double>::infinity();

        for (int row = 0; row < d; row++) {
            double rawEntropy = 0.0;

            for (int col = 0; col < m; col++) {
                double cnt =
                    static_cast<double>(
                        counter[sketchID][row][col]
                    );

                if (cnt > 0.0) {
                    rawEntropy +=
                        cnt *
                        std::log2(cnt);
                }
            }

            if (rawEntropy < minRawEntropy) {
                minRawEntropy =
                    rawEntropy;
            }
        }

        return
            std::log2(total)
            -
            minRawEntropy / total;
    }


    double estimateEntropy_(
        int sketchID,
        double total
    ) const {
        double maxEntropy = 0.0;

        for (int row = 0; row < d; row++) {
            double entropy = 0.0;

            for (int col = 0; col < m; col++) {
                double cnt =
                    static_cast<double>(
                        counter[sketchID][row][col]
                    );

                if (cnt > 0.0) {
                    double prob =
                        cnt / total;

                    entropy -=
                        prob *
                        std::log2(prob);
                }
            }

            if (entropy > maxEntropy) {
                maxEntropy = entropy;
            }
        }

        return maxEntropy;
    }


    std::vector<double> query(double p) const {
        int levels = p;

        if (packetnum == 0) {
            throw std::runtime_error(
                "CM is empty"
            );
        }

        for (
            int level = 0;
            level < levels;
            level++
        ) {
            if (
                levelPacketnum[level]
                !=
                packetnum
            ) {
                throw std::runtime_error(
                    "Not all packets were inserted into the queried levels"
                );
            }
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

        double F2_0 =
            total * total;

        F2_x.push_back(0.0);
        F2_y.push_back(
            std::log2(F2_0)
        );

        Ent_x.push_back(0.0);
        Ent_y.push_back(0.0);


        for (
            int level = 1;
            level <= levels;
            level++
        ) {
            int sketchID =
                level - 1;

            double F2 =
                estimateF2(sketchID);

            double entropy =
                estimateEntropy(
                    sketchID,
                    total
                );

            F2_x.push_back(
                static_cast<double>(
                    level
                )
            );

            F2_y.push_back(
                std::log2(
                    (
                        m * F2
                        -
                        total * total
                    )
                    /
                    (m - 1)
                )
            );

            Ent_x.push_back(
                static_cast<double>(
                    level
                )
            );

            Ent_y.push_back(
                entropy
            );

            double nodeNum =
                std::exp2(
                    static_cast<double>(
                        level
                    )
                );

            double variance =
                nodeNum * F2 /
                (total * total)
                -
                1.0;

            if (variance > 0.0) {
                Var_x.push_back(
                    -static_cast<double>(
                        level
                    )
                );

                Var_y.push_back(
                    std::log2(
                        variance
                    )
                );
            }

            cout
                << "p = "
                << level

                << " Fractal = "
                << log2(F2)

                << " Variance = "
                << log2(variance)

                << " Entropy = "
                << entropy

                << endl;
        }


        if (
            F2_x.size() < 2 ||
            Ent_x.size() < 2 ||
            Var_x.size() < 2
        ) {
            throw std::runtime_error(
                "Not enough valid levels for regression"
            );
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