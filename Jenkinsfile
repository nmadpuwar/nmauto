pipeline {

  agent any

  stages {

    stage('Checkout Source') {
      steps {
        git 'https://github.com/nmadpuwar/nmauto.git'
      }
    }

    stage('Deploy App') {
      steps {
        script {
          kubernetesDeploy(configs: "es-dep.yaml", kubeconfigId: "nmadpuwar")
        }
      }
    }

  }

}
