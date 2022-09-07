pipeline {
	agent any
	stages {
		stage("Build") {
			steps {
				sh 'cmake -B build --preset release'
        			sh 'cmake --build build'
        			sh 'tar czf cvm3.tar.gz build/*'
				archiveArtifacts artifacts: 'cvm3.tar.gz', fingerprint: true
			}
		}
	}
}
