class Automation {

    static instances = []

    constructor(data) {
        this.data = data || {}
        this.el = null
        this.init()
        Automation.instances.push(this)
    }

    init() {
        if (!this.isValid()) return
        this.el = document.createElement('div')
        this.el.setAttribute('at-id', this.data.id || '')
        this.el.className = 'card card-1 card-at mb-5'
        this.el.innerHTML = `<div class="card-body d-flex align-items-center p-4">
                <span class="fs-4 fw-medium" role="at-name">${this.data.name}</span>
                <div class="ms-auto form-check form-check-lg form-switch">
                    <input class="form-check-input" type="checkbox" action="atEnableToggle" ${this.data.enabled ? 'checked' : ''}>
                </div>
            </div>`
        this.el.addEventListener('click', (e) => {
            const action = e.target.getAttribute('action')
            if (action === 'atEnableToggle') {
                this.enable(e.target.checked)
            } else {
                new ATEdit('edit', this.data, this)
            }
        })
    }

    HTMLObject() {
        return this.el
    }

    handleChanges(data) {
        this.data = data
        if (data.lastAction == 'delete') {
            this.remove()
        } else if (data.lastAction == 'save') {
            console.log('save', data)
            this.update()
        } else if (data.lastAction == 'discard') {
            console.log('discard', data)
            if (!this.el) {
                // remove instance
            }
        } else if (data.lastAction == 'add') {
            console.log('add', data)
            this.init()
            this.appendTo($main)
            this._disableUI()
            this.add((data) => {
                if (!data || !data.success || !data.id) {
                    this.el.remove()
                    // remove instance
                    return
                }
            })
        }
    }

    _disableUI(disable = true) {
        if (!this.el) return
        if (disable) {
            this.el.style.opacity = 0.3
            this.el.style.pointerEvents = 'none'
        } else {
            this.el.style.opacity = 1
            this.el.style.pointerEvents = 'auto'
        }
    }

    isValid() {
        return this.data && this.data.name && this.data.triggers && this.data.actions && this.data.triggers.length && this.data.actions.length
    }

    enable(enable = true, callback) {
        if (!this.isValid()) return
        console.log(`enabling AT ${this.data.name}[${this.data.id}] -> ${enable}`)
        this._disableUI()
        const $el = this.el.querySelector('[action="atEnableToggle"]')
        let url = baseURI + '/auto/enable?id=' + this.data.id
        if (!enable) url += '&disable=1'
        GET(url)
            .then(data => {
                this._disableUI(false)
                if (data.success === true) {
                    notify('success', `Automation "${this.data.name}" ${enable ? 'enabled' : 'disabled'} successfully`)
                    this.data.enabled = enable
                } else {
                    notify('failure', `Failed to ${enable ? 'enable' : 'disable'} automation "${this.data.name}": ${data.message}`)
                    $el.checked = !enable
                }
                if (callback) callback(data)
            })
            .catch(err => {
                this._disableUI(false)
                notify('failure', `Failed to ${enable ? 'enable' : 'disable'} automation "${this.data.name}": ${err.message}`)
                $el.checked = !enable
                if (callback) callback(null)
            })
    }

    remove(callback) {
        if (!this.isValid()) return
        console.log(`removing AT ${this.data.name}[${this.data.id}]`)

        this._disableUI()
        Loading.circle()
        GET(baseURI + '/auto/remove?id=' + this.data.id)
            .then(data => {
                Loading.remove()
                if (data.success === true) {
                    notify('success', `Automation "${this.data.name}" deleted successfully`)
                    this.el.remove()
                    setTimeout(() => location.reload(), 1500)
                } else {
                    notify('failure', `Failed to delete automation "${this.data.name}": ${data.message}`)
                    this.el.style.opacity = 1
                }
                if (callback) callback(data)
            })
            .catch(err => {
                Loading.remove()
                this._disableUI(false)
                notify('failure', `Failed to delete automation "${this.data.name}": ${err.message}`)
                if (callback) callback(null)
            })
    }

    add(callback) {
        this.update(callback)
    }

    update(callback) {
        if (!this.isValid()) {
            notify('failure', `Automation "${this.data.name}" is not valid`)
            if (callback) callback(null)
            return
        }
        let isAdd = !this.data.id
        const notiStr = isAdd ? 'Add' : 'Updat'

        const { el, data } = this
        this._disableUI()
        let url = baseURI + '/auto/add-update'
        if (data.id) {
            url += '?id=' + data.id
        }

        // format data
        let dataStr = ''
        dataStr += `${data.enabled ? '1' : '0'}\n`
        dataStr += `${data.name}\n`
        dataStr += `${data.matchType}\n`
        for (const trigger of data.triggers) {
            if (trigger.atType == '2') {
                dataStr += `2|${trigger.did}|${trigger.prop}|${trigger.value}\n`
            }
        }
        dataStr += '\r\n'
        for (const action of data.actions) {
            if (action.atType == '2') {
                dataStr += `2|${action.did}|${action.prop}|${action.value}\n`
            }
        }
        dataStr += '\r\n'

        console.log(`${notiStr}ing AT`, data.name, data.id)
        console.log(url)
        console.log(data)
        console.log(dataStr)

        POST(url, dataStr)
            .then(jsondata => {
                this._disableUI(false)
                let isSuccess = false
                if (jsondata.success === true) {
                    if (isAdd) {
                        isSuccess = jsondata.id ? true : false
                    } else {
                        isSuccess = true
                    }
                }
                if (isSuccess) {
                    notify('success', `Automation "${data.name}" ${notiStr}ed successfully`)
                    try {
                        el.querySelector('[role="at-name"]').textContent = data.name
                        el.querySelector('[action="atEnableToggle"]').checked = data.enabled
                        if (jsondata.id) {
                            data.id = jsondata.id
                            el.setAttribute('at-id', jsondata.id)
                        }
                    } catch (e) { }
                } else {
                    notify('failure', `Failed to update automation "${data.name}": ${jsondata.message}`)
                }
                if (callback) callback(jsondata)
            })
            .catch(err => {
                this._disableUI(false)
                notify('failure', `Failed to update automation "${data.name}": ${err.message}`)
                if (callback) callback(null)
            })
    }

    appendTo(parent) {
        parent?.appendChild(this.el)
    }

}