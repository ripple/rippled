#ifndef RIPPLE_PROOFOFWORK_H
#define RIPPLE_PROOFOFWORK_H

class ProofOfWork
{
public:
	static const int sMaxDifficulty;

	typedef boost::shared_ptr <ProofOfWork> pointer;

	ProofOfWork (const std::string& token,
                 int iterations,
                 const uint256& challenge,
                 const uint256& target);

	explicit ProofOfWork (const std::string& token);

	bool isValid() const;

	uint256 solve(int maxIterations = 2 * sMaxIterations) const;
	bool checkSolution(const uint256& solution) const;

	const std::string& getToken() const		{ return mToken; }
	const uint256& getChallenge() const		{ return mChallenge; }

    uint64 getDifficulty() const
    {
        return getDifficulty(mTarget, mIterations);
    }

	// approximate number of hashes needed to solve
	static uint64 getDifficulty (const uint256& target, int iterations);

    static bool validateToken (const std::string& strToken);

private:
	std::string		mToken;
	uint256			mChallenge;
	uint256			mTarget;
	int				mIterations;

	static const uint256 sMinTarget;
	static const int sMaxIterations;
};

#endif

// vim:ts=4
