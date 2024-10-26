
let tests = [
    [-2, +2, +3, -1],
    [-3, +1, +2, -4],
    [+1, +2, +3, +4],
    [0, -1, +2, 0],
    [0, -1, -2, -2],
    [-2, -3, 1, -1],
    [0, 0, -2, 1]
]

for (let test of tests) {

    let origStr = test.join(',')
    let origSum = test.reduce((a, b) => a + b, 0)

    let positives = test.filter(x => x > 0).reduce((a, b) => a + b, 0)
    let negatives = test.filter(x => x < 0).reduce((a, b) => a - b, 0)

    let side = 1
    let correction = positives

    if (positives > negatives) {
        side = -1
        correction = negatives
    }

    for (let i = 0; i < test.length; i++) {
        if (test[i] * side > 0) {
            test[i] = 0
        }
    }

    for (let i = test.length-1; i >= 0; i--) {
        let c = Math.min(correction, Math.abs(test[i]))
        test[i] += c * side
        correction -= c
    }

    console.log(`[${origStr}] ∑=${origSum} => [${test}] ∑=${test.reduce((a, b) => a + b, 0)}`)
}



