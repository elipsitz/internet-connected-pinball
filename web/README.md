# Score Server

Deployed with `dokku`. 

Set up the `data` directory somewhere, rename the example `config.example.json` to `config.json`, and mount the directory in the app container with:

`dokku storage:mount <app> <location of data directory>:/data`
